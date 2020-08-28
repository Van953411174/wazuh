/*
 * Copyright (C) 2015-2020, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>

#include "../../wazuh_modules/wmodules.h"
#include "../../wazuh_modules/agent_upgrade/manager/wm_agent_upgrade_tasks.h"
#include "../../headers/shared.h"

#ifdef TEST_SERVER

cJSON *wm_agent_send_task_information_master(const cJSON *message_object);
cJSON *wm_agent_send_task_information_worker(const cJSON *message_object);

// Setup / teardown

static int setup_agent_task(void **state) {
    wm_agent_task *agent_task = NULL;
    agent_task = wm_agent_upgrade_init_agent_task();
    *state = (void *)agent_task;
    return 0;
}

static int teardown_agent_task(void **state) {
    wm_agent_task *agent_task = *state;
    wm_agent_upgrade_free_agent_task(agent_task);
    return 0;
}

static int setup_node(void **state) {
    OSHashNode *node = NULL;
    os_calloc(1, sizeof(OSHashNode), node);
    *state = (void *)node;
    return 0;
}

static int teardown_node(void **state) {
    OSHashNode *node = (OSHashNode *)*state;
    os_free(node);
    return 0;
}

static int teardown_jsons(void **state) {
    cJSON *json = state[0];
    cJSON *json2 = state[1];
    cJSON_Delete(json);
    cJSON_Delete(json2);
    return 0;
}

#endif

// Wrappers

void __wrap__mterror(const char *tag, const char * file, int line, const char * func, const char *msg, ...) {
    char formatted_msg[OS_MAXSTR];
    va_list args;

    check_expected(tag);

    va_start(args, msg);
    vsnprintf(formatted_msg, OS_MAXSTR, msg, args);
    va_end(args);

    check_expected(formatted_msg);
}

void __wrap__mtdebug1(const char *tag, const char * file, int line, const char * func, const char *msg, ...) {
    char formatted_msg[OS_MAXSTR];
    va_list args;

    check_expected(tag);

    va_start(args, msg);
    vsnprintf(formatted_msg, OS_MAXSTR, msg, args);
    va_end(args);

    check_expected(formatted_msg);
}

int __wrap_OSHash_Add_ex(OSHash *self, const char *key, void *data) {
    check_expected(key);
    check_expected(data);

    return mock();
}

void *__wrap_OSHash_Get_ex(const OSHash *self, const char *key) {
    check_expected(key);

    return mock_type(void*);
}

int __wrap_OSHash_Update_ex(OSHash *self, const char *key, void *data) {
    check_expected(key);
    check_expected(data);

    return mock();
}

void *__wrap_OSHash_Delete_ex(OSHash *self, const char *key) {
    check_expected(key);

    return mock_type(void*);
}

int __wrap_OSHash_Begin(unsigned int *index) {
    check_expected(index);

    return mock();
}

int __wrap_OSHash_Next(unsigned int *index, OSHashNode *current) {
    check_expected(index);

    return mock();
}

int __wrap_OS_ConnectUnixDomain(const char *path, int type, int max_msg_size) {
    check_expected(path);
    check_expected(type);
    check_expected(max_msg_size);

    return mock();
}

int __wrap_OS_SendSecureTCP(int sock, uint32_t size, const void * msg) {
    check_expected(sock);
    check_expected(size);
    if (msg) check_expected(msg);

    return mock();
}

int __wrap_OS_RecvSecureTCP(int sock, char *ret, uint32_t size) {
    check_expected(sock);
    check_expected(size);

    if (mock()) {
        strncpy(ret, mock_type(char*), size);
    }

    return mock();
}

int __wrap_close(int fd) {
    check_expected(fd);
    return 0;
}

cJSON* __wrap_w_create_sendsync_payload(const char *daemon_name, cJSON *message) {
    check_expected(daemon_name);

    return mock_type(cJSON*);
}

int __wrap_w_send_clustered_message(const char* command, const char* payload, char* response) {
    check_expected(command);
    check_expected(payload);

    strcpy(response, mock_type(char*));

    return mock();
}

int __wrap_w_is_worker(void) {
    return mock();
}

cJSON* __wrap_cJSON_Duplicate(const cJSON *item, int recurse) {
    return mock_type(cJSON*);
}

#ifdef TEST_SERVER

// Tests

void test_wm_agent_upgrade_create_task_entry_ok(void **state)
{
    int agent_id = 6;
    wm_agent_task *agent_task = *state;

    expect_string(__wrap_OSHash_Add_ex, key, "6");
    expect_memory(__wrap_OSHash_Add_ex, data, agent_task, sizeof(agent_task));
    will_return(__wrap_OSHash_Add_ex, OSHASH_SUCCESS);

    int ret = wm_agent_upgrade_create_task_entry(agent_id, agent_task);

    assert_int_equal(ret, OSHASH_SUCCESS);
}

void test_wm_agent_upgrade_create_task_entry_duplicate(void **state)
{
    int agent_id = 6;
    wm_agent_task *agent_task = *state;

    expect_string(__wrap_OSHash_Add_ex, key, "6");
    expect_memory(__wrap_OSHash_Add_ex, data, agent_task, sizeof(agent_task));
    will_return(__wrap_OSHash_Add_ex, OSHASH_DUPLICATED);

    int ret = wm_agent_upgrade_create_task_entry(agent_id, agent_task);

    assert_int_equal(ret, OSHASH_DUPLICATED);
}

void test_wm_agent_upgrade_insert_task_id_ok(void **state)
{
    int agent_id = 8;
    int task_id = 100;
    wm_agent_task *agent_task = *state;

    agent_task->task_info = wm_agent_upgrade_init_task_info();

    expect_string(__wrap_OSHash_Get_ex, key, "8");
    will_return(__wrap_OSHash_Get_ex, agent_task);

    expect_string(__wrap_OSHash_Update_ex, key, "8");
    expect_memory(__wrap_OSHash_Update_ex, data, agent_task, sizeof(agent_task));
    will_return(__wrap_OSHash_Update_ex, OSHASH_SUCCESS);

    wm_agent_upgrade_insert_task_id(agent_id, task_id);

    assert_int_equal(agent_task->task_info->task_id, task_id);
}

void test_wm_agent_upgrade_insert_task_id_err(void **state)
{
    int agent_id = 8;
    int task_id = 100;

    expect_string(__wrap_OSHash_Get_ex, key, "8");
    will_return(__wrap_OSHash_Get_ex, NULL);

    wm_agent_upgrade_insert_task_id(agent_id, task_id);
}

void test_wm_agent_upgrade_remove_entry_ok(void **state)
{
    int agent_id = 10;
    wm_agent_task *agent_task = *state;

    expect_string(__wrap_OSHash_Delete_ex, key, "10");
    will_return(__wrap_OSHash_Delete_ex, agent_task);

    wm_agent_upgrade_remove_entry(agent_id);
}

void test_wm_agent_upgrade_remove_entry_err(void **state)
{
    int agent_id = 10;

    expect_string(__wrap_OSHash_Delete_ex, key, "10");
    will_return(__wrap_OSHash_Delete_ex, NULL);

    wm_agent_upgrade_remove_entry(agent_id);
}

void test_wm_agent_upgrade_get_first_node(void **state)
{
    int index = 0;

    expect_value(__wrap_OSHash_Begin, index, index);
    will_return(__wrap_OSHash_Begin, 1);

    OSHashNode* ret = wm_agent_upgrade_get_first_node(&index);

    assert_int_equal(ret, 1);
}

void test_wm_agent_upgrade_get_next_node(void **state)
{
    int index = 0;
    OSHashNode *node = *state;

    expect_value(__wrap_OSHash_Next, index, index);
    will_return(__wrap_OSHash_Next, 1);

    OSHashNode* ret = wm_agent_upgrade_get_next_node(&index, node);

    assert_int_equal(ret, 1);
}

void test_wm_agent_send_task_information_master_ok(void **state)
{
    int socket = 555;

    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    char *message = "[{\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":12},{"
                      "\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":10}]";

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, socket);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8157): Sending message to task_manager module: '[{\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":12},{"
                                                                                                      "\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":10}]'");

    expect_value(__wrap_OS_SendSecureTCP, sock, socket);
    expect_value(__wrap_OS_SendSecureTCP, size, strlen(message));
    expect_string(__wrap_OS_SendSecureTCP, msg, message);
    will_return(__wrap_OS_SendSecureTCP, 0);

    expect_value(__wrap_OS_RecvSecureTCP, sock, socket);
    expect_value(__wrap_OS_RecvSecureTCP, size, OS_MAXSTR);
    will_return(__wrap_OS_RecvSecureTCP, 1);
    will_return(__wrap_OS_RecvSecureTCP, response);
    will_return(__wrap_OS_RecvSecureTCP, strlen(response) + 1);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8158): Receiving message from task_manager module: '[{\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":12,"
                                                                                                          "\"task_id\":100},{"
                                                                                                          "\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":10,"
                                                                                                          "\"task_id\":101}]'");

    expect_value(__wrap_close, fd, socket);

    cJSON *output = wm_agent_send_task_information_master(input);

    state[0] = output;
    state[1] = input;

    assert_non_null(output);
}

void test_wm_agent_send_task_information_master_json_err(void **state)
{
    int socket = 555;

    char *response = "wrong json";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    char *message = "[{\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":12},{"
                      "\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":10}]";

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, socket);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8157): Sending message to task_manager module: '[{\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":12},{"
                                                                                                      "\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":10}]'");

    expect_value(__wrap_OS_SendSecureTCP, sock, socket);
    expect_value(__wrap_OS_SendSecureTCP, size, strlen(message));
    expect_string(__wrap_OS_SendSecureTCP, msg, message);
    will_return(__wrap_OS_SendSecureTCP, 0);

    expect_value(__wrap_OS_RecvSecureTCP, sock, socket);
    expect_value(__wrap_OS_RecvSecureTCP, size, OS_MAXSTR);
    will_return(__wrap_OS_RecvSecureTCP, 1);
    will_return(__wrap_OS_RecvSecureTCP, response);
    will_return(__wrap_OS_RecvSecureTCP, strlen(response) + 1);

    expect_string(__wrap__mterror, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mterror, formatted_msg, "(8105): Response from task manager does not have a valid JSON format.");

    expect_value(__wrap_close, fd, socket);

    cJSON *output = wm_agent_send_task_information_master(input);

    state[0] = output;
    state[1] = input;

    assert_null(output);
}

void test_wm_agent_send_task_information_master_recv_error(void **state)
{
    int socket = 555;

    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    char *message = "[{\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":12},{"
                      "\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":10}]";

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, socket);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8157): Sending message to task_manager module: '[{\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":12},{"
                                                                                                      "\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":10}]'");

    expect_value(__wrap_OS_SendSecureTCP, sock, socket);
    expect_value(__wrap_OS_SendSecureTCP, size, strlen(message));
    expect_string(__wrap_OS_SendSecureTCP, msg, message);
    will_return(__wrap_OS_SendSecureTCP, 0);

    expect_value(__wrap_OS_RecvSecureTCP, sock, socket);
    expect_value(__wrap_OS_RecvSecureTCP, size, OS_MAXSTR);
    will_return(__wrap_OS_RecvSecureTCP, 1);
    will_return(__wrap_OS_RecvSecureTCP, response);
    will_return(__wrap_OS_RecvSecureTCP, -1);

    expect_string(__wrap__mterror, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mterror, formatted_msg, "(8111): Error in recv(): 'Success'");

    expect_value(__wrap_close, fd, socket);

    cJSON *output = wm_agent_send_task_information_master(input);

    state[0] = output;
    state[1] = input;

    assert_null(output);
}

void test_wm_agent_send_task_information_master_sockterr_error(void **state)
{
    int socket = 555;

    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    char *message = "[{\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":12},{"
                      "\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":10}]";

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, socket);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8157): Sending message to task_manager module: '[{\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":12},{"
                                                                                                      "\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":10}]'");

    expect_value(__wrap_OS_SendSecureTCP, sock, socket);
    expect_value(__wrap_OS_SendSecureTCP, size, strlen(message));
    expect_string(__wrap_OS_SendSecureTCP, msg, message);
    will_return(__wrap_OS_SendSecureTCP, 0);

    expect_value(__wrap_OS_RecvSecureTCP, sock, socket);
    expect_value(__wrap_OS_RecvSecureTCP, size, OS_MAXSTR);
    will_return(__wrap_OS_RecvSecureTCP, 1);
    will_return(__wrap_OS_RecvSecureTCP, response);
    will_return(__wrap_OS_RecvSecureTCP, OS_SOCKTERR);

    expect_string(__wrap__mterror, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mterror, formatted_msg, "(8112): Response size is bigger than expected.");

    expect_value(__wrap_close, fd, socket);

    cJSON *output = wm_agent_send_task_information_master(input);

    state[0] = output;
    state[1] = input;

    assert_null(output);
}

void test_wm_agent_send_task_information_master_connect_error(void **state)
{
    int socket = 555;

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, OS_SOCKTERR);

    expect_string(__wrap__mterror, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mterror, formatted_msg, "(8104): Cannot connect to '/var/ossec/queue/tasks/task'. Could not reach task manager module.");

    cJSON *output = wm_agent_send_task_information_master(input);

    state[0] = output;
    state[1] = input;

    assert_null(output);
}

void test_wm_agent_send_task_information_worker(void **state)
{
    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    cJSON * cluster_request = cJSON_CreateObject();

    cJSON_AddStringToObject(cluster_request, "daemon_name", TASK_MANAGER_WM_NAME);
    cJSON_AddItemToObject(cluster_request, "message", input);

    char *message = "{\"daemon_name\":\"task-manager\","
                     "\"message\":[{\"module\":\"upgrade_module\","
                                   "\"command\":\"upgrade\","
                                   "\"agent\":12},{"
                                   "\"module\":\"upgrade_module\","
                                   "\"command\":\"upgrade\","
                                   "\"agent\":10}]}";

    will_return(__wrap_cJSON_Duplicate, input);

    expect_string(__wrap_w_create_sendsync_payload, daemon_name, TASK_MANAGER_WM_NAME);
    will_return(__wrap_w_create_sendsync_payload, cluster_request);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8168): Sending sendsync message to task manager in master node: '{\"daemon_name\":\"task-manager\","
                                                                                                                      "\"message\":[{\"module\":\"upgrade_module\","
                                                                                                                                    "\"command\":\"upgrade\","
                                                                                                                                    "\"agent\":12},{"
                                                                                                                                    "\"module\":\"upgrade_module\","
                                                                                                                                    "\"command\":\"upgrade\","
                                                                                                                                    "\"agent\":10}]}'");

    expect_string(__wrap_w_send_clustered_message, command, "sendsync");
    expect_string(__wrap_w_send_clustered_message, payload, message);
    will_return(__wrap_w_send_clustered_message, response);
    will_return(__wrap_w_send_clustered_message, 1);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8158): Receiving message from task_manager module: '[{\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":12,"
                                                                                                          "\"task_id\":100},{"
                                                                                                          "\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":10,"
                                                                                                          "\"task_id\":101}]'");

    cJSON *output = wm_agent_send_task_information_worker(input);

    state[0] = output;

    assert_non_null(output);
}

void test_wm_agent_upgrade_send_tasks_information_master(void **state)
{
    int socket = 555;

    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    char *message = "[{\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":12},{"
                      "\"module\":\"upgrade_module\","
                      "\"command\":\"upgrade\","
                      "\"agent\":10}]";

    will_return(__wrap_w_is_worker, 0);

    expect_string(__wrap_OS_ConnectUnixDomain, path, DEFAULTDIR TASK_QUEUE);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);
    will_return(__wrap_OS_ConnectUnixDomain, socket);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8157): Sending message to task_manager module: '[{\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":12},{"
                                                                                                      "\"module\":\"upgrade_module\","
                                                                                                      "\"command\":\"upgrade\","
                                                                                                      "\"agent\":10}]'");

    expect_value(__wrap_OS_SendSecureTCP, sock, socket);
    expect_value(__wrap_OS_SendSecureTCP, size, strlen(message));
    expect_string(__wrap_OS_SendSecureTCP, msg, message);
    will_return(__wrap_OS_SendSecureTCP, 0);

    expect_value(__wrap_OS_RecvSecureTCP, sock, socket);
    expect_value(__wrap_OS_RecvSecureTCP, size, OS_MAXSTR);
    will_return(__wrap_OS_RecvSecureTCP, 1);
    will_return(__wrap_OS_RecvSecureTCP, response);
    will_return(__wrap_OS_RecvSecureTCP, strlen(response) + 1);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8158): Receiving message from task_manager module: '[{\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":12,"
                                                                                                          "\"task_id\":100},{"
                                                                                                          "\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":10,"
                                                                                                          "\"task_id\":101}]'");

    expect_value(__wrap_close, fd, socket);

    cJSON *output = wm_agent_upgrade_send_tasks_information(input);

    state[0] = output;
    state[1] = input;

    assert_non_null(output);
}

void test_wm_agent_upgrade_send_tasks_information_worker(void **state)
{
    char *response = "[{\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":12,"
                       "\"task_id\":100},{"
                       "\"error\":0,"
                       "\"data\":\"Success\","
                       "\"agent\":10,"
                       "\"task_id\":101}]";

    cJSON *input = cJSON_CreateArray();

    cJSON *task_request1 = cJSON_CreateObject();
    cJSON *task_request2 = cJSON_CreateObject();

    cJSON_AddStringToObject(task_request1, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request1, "command", "upgrade");
    cJSON_AddNumberToObject(task_request1, "agent", 12);

    cJSON_AddStringToObject(task_request2, "module", "upgrade_module");
    cJSON_AddStringToObject(task_request2, "command", "upgrade");
    cJSON_AddNumberToObject(task_request2, "agent", 10);

    cJSON_AddItemToArray(input, task_request1);
    cJSON_AddItemToArray(input, task_request2);

    cJSON * cluster_request = cJSON_CreateObject();

    cJSON_AddStringToObject(cluster_request, "daemon_name", TASK_MANAGER_WM_NAME);
    cJSON_AddItemToObject(cluster_request, "message", input);

    char *message = "{\"daemon_name\":\"task-manager\","
                     "\"message\":[{\"module\":\"upgrade_module\","
                                   "\"command\":\"upgrade\","
                                   "\"agent\":12},{"
                                   "\"module\":\"upgrade_module\","
                                   "\"command\":\"upgrade\","
                                   "\"agent\":10}]}";

    will_return(__wrap_w_is_worker, 1);

    will_return(__wrap_cJSON_Duplicate, input);

    expect_string(__wrap_w_create_sendsync_payload, daemon_name, TASK_MANAGER_WM_NAME);
    will_return(__wrap_w_create_sendsync_payload, cluster_request);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8168): Sending sendsync message to task manager in master node: '{\"daemon_name\":\"task-manager\","
                                                                                                                      "\"message\":[{\"module\":\"upgrade_module\","
                                                                                                                                    "\"command\":\"upgrade\","
                                                                                                                                    "\"agent\":12},{"
                                                                                                                                    "\"module\":\"upgrade_module\","
                                                                                                                                    "\"command\":\"upgrade\","
                                                                                                                                    "\"agent\":10}]}'");

    expect_string(__wrap_w_send_clustered_message, command, "sendsync");
    expect_string(__wrap_w_send_clustered_message, payload, message);
    will_return(__wrap_w_send_clustered_message, response);
    will_return(__wrap_w_send_clustered_message, 1);

    expect_string(__wrap__mtdebug1, tag, "wazuh-modulesd:agent-upgrade");
    expect_string(__wrap__mtdebug1, formatted_msg, "(8158): Receiving message from task_manager module: '[{\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":12,"
                                                                                                          "\"task_id\":100},{"
                                                                                                          "\"error\":0,"
                                                                                                          "\"data\":\"Success\","
                                                                                                          "\"agent\":10,"
                                                                                                          "\"task_id\":101}]'");

    cJSON *output = wm_agent_upgrade_send_tasks_information(input);

    state[0] = output;

    assert_non_null(output);
}

#endif

int main(void) {
    const struct CMUnitTest tests[] = {
#ifdef TEST_SERVER
        // wm_agent_upgrade_upgrade_success_callback
        cmocka_unit_test_setup_teardown(test_wm_agent_upgrade_create_task_entry_ok, setup_agent_task, teardown_agent_task),
        cmocka_unit_test_setup_teardown(test_wm_agent_upgrade_create_task_entry_duplicate, setup_agent_task, teardown_agent_task),
        // wm_agent_upgrade_insert_task_id
        cmocka_unit_test_setup_teardown(test_wm_agent_upgrade_insert_task_id_ok, setup_agent_task, teardown_agent_task),
        cmocka_unit_test(test_wm_agent_upgrade_insert_task_id_err),
        // wm_agent_upgrade_remove_entry
        cmocka_unit_test_setup(test_wm_agent_upgrade_remove_entry_ok, setup_agent_task),
        cmocka_unit_test(test_wm_agent_upgrade_remove_entry_err),
        // wm_agent_upgrade_get_first_node
        cmocka_unit_test(test_wm_agent_upgrade_get_first_node),
        // wm_agent_upgrade_get_next_node
        cmocka_unit_test_setup_teardown(test_wm_agent_upgrade_get_next_node, setup_node, teardown_node),
        // wm_agent_send_task_information_master
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_master_ok, teardown_jsons),
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_master_json_err, teardown_jsons),
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_master_recv_error, teardown_jsons),
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_master_sockterr_error, teardown_jsons),
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_master_connect_error, teardown_jsons),
        // wm_agent_send_task_information_worker
        cmocka_unit_test_teardown(test_wm_agent_send_task_information_worker, teardown_jsons),
        // wm_agent_upgrade_send_tasks_information
        cmocka_unit_test_teardown(test_wm_agent_upgrade_send_tasks_information_master, teardown_jsons),
        cmocka_unit_test_teardown(test_wm_agent_upgrade_send_tasks_information_worker, teardown_jsons),
#endif
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
