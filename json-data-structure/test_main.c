#include "json.h"
#include "xtest.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>

//  完成使用场景的测试
TEST(test, scene)
{
    JSON *json = json_new(JSON_OBJ);
    ASSERT_TRUE(json != NULL);
    JSON *basic = json_new(JSON_OBJ);
    ASSERT_TRUE(basic != NULL);

    ASSERT_TRUE(NULL != json_add_member(json, "basic", basic));

    ASSERT_TRUE(NULL != json_add_member(basic, "enable", json_new_bool(TRUE)));
    EXPECT_EQ(TRUE, json_obj_get_bool(basic, "enable"));

    ASSERT_TRUE(NULL != json_add_member(basic, "port", json_new_num(389)));
    EXPECT_EQ(389, json_obj_get_num(basic, "port", 0));
//...
    ASSERT_TRUE(NULL != json_add_member(basic, "ip", json_new_str("200.200.3.61")));
    const char *ip = json_obj_get_str(basic, "ip", NULL);
    ASSERT_STRCASEEQ("200.200.3.61", ip);

    json_free(json);
}

//  测试键值对存在的情况
TEST(json_obj_get_str, exist)
{
    JSON *json = json_new(JSON_OBJ);
    ASSERT_TRUE(json != NULL);

    ASSERT_TRUE(NULL != json_add_member(json, "ip", json_new_str("200.200.3.61")));
    const char *ip = json_obj_get_str(json, "ip", NULL);
    ASSERT_TRUE(ip != NULL);
    ASSERT_STRCASEEQ("200.200.3.61", ip);

    json_free(json);
}

//  测试键值对不存在的情况
TEST(json_obj_get_str, notexist)
{
    JSON *json = json_new(JSON_OBJ);
    ASSERT_TRUE(json != NULL);

    ASSERT_TRUE(NULL != json_add_member(json, "ip", json_new_str("200.200.3.61")));
    const char *ip = json_obj_get_str(json, "ip2", NULL);
    ASSERT_TRUE(ip == NULL);

    ip = json_obj_get_str(json, "ip3", "default");
    ASSERT_TRUE(ip != NULL);
    ASSERT_STRCASEEQ("default", ip);

    json_free(json);
}

//----------------------------------------------------------------------------------------------------
//  json_save
//----------------------------------------------------------------------------------------------------

typedef struct buf_t {
    char *str;
    unsigned int size;
} buf_t;

int read_file(buf_t *buf, const char *fname)
{
    FILE *fp;
    long len;
    long realsize;

    assert(buf);
    assert(fname);
    assert(fname[0]);

    fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "open file [%s] failed\n", fname);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if (len <= 0) {
        fclose(fp);
        fprintf(stderr, "ftell failed, errno: %d\n", errno);
        return -1;
    }
    fseek(fp, 0, SEEK_SET);
    buf->str = (char *)malloc(len + 1);
    if (!buf->str) {
        fclose(fp);
        buf->size = 0;
        fprintf(stderr, "malloc(%ld) failed\n", len + 1);
        return -1;
    }
    buf->size = len + 1;
    realsize = fread(buf->str, 1, len, fp);
    fclose(fp);

    buf->str[realsize] = '\0';
    return 0;
}

TEST(json_save, str)
{
    JSON *json;
    buf_t result;
    const char *expect = "hello world";

    json = json_new_str("hello world");
    EXPECT_EQ(0, json_save(json, "test.yml"));
    EXPECT_EQ(0, read_file(&result, "test.yml"));

    ASSERT_TRUE(strcmp(result.str, expect) == 0);
    free(result.str);
    json_free(json);
}

TEST(json_save, special_str)
{
    JSON *json;
    buf_t result;
    const char *expect = "hello\nworld";

    json = json_new_str("hello\nworld");
    EXPECT_EQ(0, json_save(json, "test.yml"));
    EXPECT_EQ(0, read_file(&result, "test.yml"));

    ASSERT_TRUE(strcmp(result.str, expect) == 0);
    free(result.str);
    json_free(json);
}

TEST(json_save, obj)
{
    JSON *json;
    buf_t result;
    const char *expect = "key: hello\nname: world";

    json = json_new(JSON_OBJ);
    json_add_member(json, "key", json_new_str("hello"));
    json_add_member(json, "name", json_new_str("world"));

    EXPECT_EQ(0, json_save(json, "test-obj.yml"));
    EXPECT_EQ(0, read_file(&result, "test-obj.yml"));

    ASSERT_TRUE(strcmp(result.str, expect) == 0);
    free(result.str);
    json_free(json);
}

TEST(json_save, json_none_outputs_null) {
    // 准备测试数据
    JSON *json = json_new(JSON_NONE); // 假设有 json_new 函数创建 JSON_NONE 对象
    EXPECT_NE(json, NULL);

    // 调用被测函数
    EXPECT_EQ(0, json_save(json, "test_none.yml"));

    // 验证文件内容
    buf_t result;
    EXPECT_EQ(0, read_file(&result, "test_none.yml"));
    EXPECT_STREQ("null", result.str);

    // 清理
    free(result.str);
    json_free(json);
}

// 测试空数组
TEST(json_save, json_arr_empty_outputs_brackets) {
    JSON *json = json_new(JSON_ARR); // 创建空数组
    EXPECT_NE(json, NULL);

    EXPECT_EQ(0, json_save(json, "test_arr_empty.yml"));

    buf_t result;
    EXPECT_EQ(0, read_file(&result, "test_arr_empty.yml"));
    EXPECT_STREQ("[]", result.str);

    free(result.str);
    json_free(json);
}

// 测试非空数组
TEST(json_save, json_arr_non_empty_outputs_yaml_list) {
    JSON *json = json_new(JSON_ARR);
    EXPECT_NE(json, NULL);

    // 添加两个元素到数组
    JSON *elem1 = json_new_num(42);
    JSON *elem2 = json_new_num(3.14);
    json_add_element(json, elem1);
    json_add_element(json, elem2);

    EXPECT_EQ(0, json_save(json, "test_arr_non_empty.yml"));

    // 验证文件内容
    buf_t result;
    EXPECT_EQ(0, read_file(&result, "test_arr_non_empty.yml"));

    const char *expected = 
        "\n"
        "- 42\n"
        "- 3.14";
    ASSERT_TRUE(strcmp(result.str, expected) == 0);
    free(result.str);
    json_free(json);
}

TEST(json_save, all_test) {
    // 直接构建JSON对象
    JSON *json = json_new(JSON_OBJ);
    EXPECT_NE(json, NULL);
    
    // 构建basic部分
    JSON *basic = json_new(JSON_OBJ);
    json_add_member(json, "basic", basic);
    json_add_member(basic, "enable", json_new_bool(TRUE));
    json_add_member(basic, "ip", json_new_str("200.200.3.61"));
    json_add_member(basic, "port", json_new_num(389));
    json_add_member(basic, "timeout", json_new_num(10));
    json_add_member(basic, "basedn", json_new_str("aaa"));
    json_add_member(basic, "fd", json_new_num(-1));
    json_add_member(basic, "maxcnt", json_new_num(133333333333));
    
    // 构建basic.dns数组
    JSON *dns = json_new(JSON_ARR);
    json_add_element(dns, json_new_str("200.200.0.1"));
    json_add_element(dns, json_new_str("200.0.0.254"));
    json_add_member(basic, "dns", dns);
    
    // 构建advance部分
    JSON *advance = json_new(JSON_OBJ);
    json_add_member(json, "advance", advance);
    
    // 构建advance.dns数组
    JSON *adv_dns = json_new(JSON_ARR);
    JSON *huanan = json_new(JSON_OBJ);
    json_add_member(huanan, "name", json_new_str("huanan"));
    json_add_member(huanan, "ip", json_new_str("200.200.0.1"));
    JSON *huabei = json_new(JSON_OBJ);
    json_add_member(huabei, "name", json_new_str("huabei"));
    json_add_member(huabei, "ip", json_new_str("200.0.0.254"));
    json_add_element(adv_dns, huanan);
    json_add_element(adv_dns, huabei);
    json_add_member(advance, "dns", adv_dns);
    
    // 构建advance.portpool数组
    JSON *portpool = json_new(JSON_ARR);
    json_add_element(portpool, json_new_num(130));
    json_add_element(portpool, json_new_num(131));
    json_add_element(portpool, json_new_num(132));
    json_add_member(advance, "portpool", portpool);
    
    // 添加其他advance字段
    json_add_member(advance, "url", json_new_str("http://200.200.0.4/main"));
    json_add_member(advance, "path", json_new_str("/etc/sinfors"));
    json_add_member(advance, "value", json_new_num(3.14));
    
    // 保存为YAML文件
    EXPECT_EQ(0, json_save(json, "test_all.yml"));
    
    // 验证文件内容
    buf_t result;
    EXPECT_EQ(0, read_file(&result, "test_all.yml"));
    
    free(result.str);
    json_free(json);
}
TEST(json_basic, creation) {
    // 测试基础类型创建
    JSON *num = json_new_num(3.14);
    ASSERT_TRUE(num != NULL);
    ASSERT_EQ(JSON_NUM, json_type(num));
    ASSERT_EQ(3.14, json_num(num, 0));
    
    JSON *str = json_new_str("hello");
    ASSERT_TRUE(str != NULL);
    ASSERT_STREQ("hello", json_str(str, NULL));
    
    JSON *bol = json_new_bool(TRUE);
    ASSERT_TRUE(bol != NULL);
    ASSERT_EQ(TRUE, json_bool(bol));
    
    json_free(num);
    json_free(str);
    json_free(bol);
}


TEST(json_object, basic_ops) {
    JSON *obj = json_new(JSON_OBJ);
    ASSERT_TRUE(obj != NULL);
    
    // 测试添加成员
    ASSERT_TRUE(json_add_member(obj, "port", json_new_num(80)) != NULL);
    ASSERT_TRUE(json_add_member(obj, "active", json_new_bool(TRUE)) != NULL);
    
    // 验证获取成员
    ASSERT_EQ(80, json_obj_get_num(obj, "port", 0));
    ASSERT_EQ(TRUE, json_obj_get_bool(obj, "active"));
    
    // 测试重复键
    ASSERT_TRUE(json_add_member(obj, "port", json_new_num(8080)) == NULL);
    
    json_free(obj);
}

TEST(json_array, basic_ops) {
    JSON *arr = json_new(JSON_ARR);
    ASSERT_TRUE(arr != NULL);
    
    // 测试添加元素
    ASSERT_TRUE(json_add_element(arr, json_new_num(1)) != NULL);
    ASSERT_TRUE(json_add_element(arr, json_new_str("text")) != NULL);
    // 验证数组内容
    ASSERT_EQ(2, json_arr_count(arr));
    ASSERT_EQ(1, json_arr_get_num(arr, 0, 0));
    ASSERT_STREQ("text", json_arr_get_str(arr, 1, NULL));
    
    json_free(arr);
}

TEST(json_array, invalid_access) {
    JSON *arr = json_new(JSON_ARR);
    // 测试越界访问
    ASSERT_EQ(0, json_arr_get_num(arr, 999, 0));
    json_free(arr);
}
TEST(json_nested, complex) {
    JSON *root = json_new(JSON_OBJ);
    JSON *services = json_new(JSON_ARR);
    
    // 构建嵌套结构
    JSON *service1 = json_new(JSON_OBJ);
    json_add_member(service1, "port", json_new_num(80));
    json_add_element(services, service1);
    json_add_member(root, "services", services);
    
    // 验证嵌套访问
    ASSERT_EQ(80, json_obj_get_num(
        json_get_element(
            json_get_member(root, "services"), 
            0
        ), 
        "port", 
        0
    ));
    
    json_free(root);
}

TEST(json_io, invalid_path) {
    ASSERT_NE(0, json_save(NULL, "test.json"));
    ASSERT_NE(0, json_save(json_new_num(1), "/invalid/path"));
}

TEST(json_array, add_and_get) {
    JSON *arr = json_new(JSON_ARR);
    json_arr_add_num(arr, 3.14);
    json_arr_add_str(arr, "text");
    
    ASSERT_EQ(3.14, json_arr_get_num(arr, 0, 0));
    ASSERT_STREQ("text", json_arr_get_str(arr, 1, NULL));
    json_free(arr);
}

TEST(json_object, set_and_get) {
    JSON *obj = json_new(JSON_OBJ);
    json_obj_set_str(obj, "name", "Alice");
    json_obj_set_num(obj, "age", 30);
    json_obj_set_str(obj, "name", "Alice");
    ASSERT_STREQ("Alice", json_obj_get_str(obj, "name", NULL));
    ASSERT_EQ(30, json_obj_get_num(obj, "age", 0));
    json_free(obj);
}


TEST(json_object, set_bool) {
    // 创建测试对象
    JSON *obj = json_new(JSON_OBJ);
    EXPECT_NE(obj, NULL);
    // 测试添加新布尔值
    EXPECT_EQ(0, json_obj_set_bool(obj, "enabled", TRUE));
    EXPECT_EQ(TRUE, json_obj_get_bool(obj, "enabled"));

    // 测试修改现有布尔值
    EXPECT_EQ(0, json_obj_set_bool(obj, "enabled", FALSE));
    EXPECT_EQ(FALSE, json_obj_get_bool(obj, "enabled"));

    // 测试错误情况
    EXPECT_EQ(-1, json_obj_set_bool(NULL, "key", TRUE));  // NULL对象
    EXPECT_EQ(-1, json_obj_set_bool(obj, NULL, TRUE));    // NULL键
    JSON *not_obj = json_new_num(1);
    EXPECT_EQ(-1, json_obj_set_bool(not_obj, "key", TRUE)); // 非对象类型

    // 测试类型冲突（尝试将非布尔成员改为布尔值）
    ASSERT_TRUE(json_add_member(obj, "name", json_new_str("test"))!=NULL);
    EXPECT_EQ(-1, json_obj_set_bool(obj, "name", TRUE));  // 类型不匹配

    json_free(obj);
    json_free(not_obj);
}

TEST(json_array, get_bool) {
    // 创建测试数组 [true, false, "not bool", 123]
    JSON *arr = json_new(JSON_ARR);
    EXPECT_NE(arr, NULL);

    EXPECT_EQ(0, json_arr_add_bool(arr, TRUE));
    EXPECT_EQ(0, json_arr_add_bool(arr, FALSE));
    EXPECT_EQ(0, json_arr_add_str(arr, "not bool"));
    EXPECT_EQ(0, json_arr_add_num(arr, 123));

    // 测试正常获取
    EXPECT_EQ(TRUE, json_arr_get_bool(arr, 0));
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, 1));

    // 测试非法情况
    EXPECT_EQ(FALSE, json_arr_get_bool(NULL, 0));   // NULL数组
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, -1));   // 负索引
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, 4));    // 越界索引
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, 2));    // 非布尔类型
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, 3));    // 非布尔类型

    // 测试非数组类型
    JSON *not_arr = json_new_num(1);
    EXPECT_EQ(FALSE, json_arr_get_bool(not_arr, 0));

    json_free(arr);
    json_free(not_arr);
}

TEST(json_array, add_bool) {
    JSON *arr = json_new(JSON_ARR);
    EXPECT_NE(arr, NULL);

    // 测试正常添加
    EXPECT_EQ(0, json_arr_add_bool(arr, TRUE));
    EXPECT_EQ(0, json_arr_add_bool(arr, FALSE));
    EXPECT_EQ(2, json_arr_count(arr));

    // 验证添加的值
    EXPECT_EQ(TRUE, json_arr_get_bool(arr, 0));
    EXPECT_EQ(FALSE, json_arr_get_bool(arr, 1));

    // 测试错误情况
    EXPECT_EQ(-1, json_arr_add_bool(NULL, TRUE));   // NULL数组
    JSON *not_arr = json_new_str("not array");
    EXPECT_EQ(-1, json_arr_add_bool(not_arr, TRUE)); // 非数组类型

    json_free(arr);
    json_free(not_arr);
}
int main(int argc, char **argv)
{
	return xtest_start_test(argc, argv);
}

