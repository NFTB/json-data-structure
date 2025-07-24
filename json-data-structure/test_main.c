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

TEST(json_get, self)
{
    JSON *json = json_new_num(20);
    const JSON *ret;

    ASSERT_TRUE(json != NULL);

    ret = json_get(json, "");
    ASSERT_TRUE(json == ret);

    ret = json_get(json, "hello");
    ASSERT_TRUE(ret == NULL);

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
    json_add_element(json, elem1); // 假设有 json_add_element 函数
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
    
    ASSERT_STREQ("Alice", json_obj_get_str(obj, "name", NULL));
    ASSERT_EQ(30, json_obj_get_num(obj, "age", 0));
    json_free(obj);
}

int main(int argc, char **argv)
{
	return xtest_start_test(argc, argv);
}

