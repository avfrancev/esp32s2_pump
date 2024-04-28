#pragma once


#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include <esp_http_server.h>
#include "typedefs.h"

#include "Pump.h"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (128 + 128)
#define SCRATCH_BUFSIZE (10240)


static inline bool file_exist(const char *path)
{
  FILE* f = fopen(path, "r");
  if (f == NULL) {
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}


#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    if (CHECK_FILE_EXTENSION(filepath, ".gz")) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".html.gz")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".json")) {
        type = "application/json";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

char chunk[1024] = { 0 };




static esp_err_t send_file(httpd_req_t *req, const char* filepath)
{
    int fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) {
        return ESP_FAIL;
    }
    Serial.printf("Sending file: %s\n", filepath);
    set_content_type_from_file(req, filepath);

    /* Send file in chunks */
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, 1024);
        if (read_bytes == -1) {
            close(fd);
            Serial.println("Failed to read file");
            /* Abort sending file */
            httpd_resp_sendstr_chunk(req, NULL);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
        /* Send the buffer contents as HTTP response chunk */
        if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
            close(fd);
            Serial.println("File sending failed!");
            /* Abort sending file */
            httpd_resp_sendstr_chunk(req, NULL);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
    } while (read_bytes > 0);

    close(fd);
    Serial.println("File sending complete");

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
  const char* uri = req->uri;
  char filepath[FILE_PATH_MAX] = "/spiffs/www";
  strlcat(filepath, uri, sizeof(filepath));

  Serial.printf("File requested: %s\n", filepath);

  struct stat st;
  if (stat(filepath, &st) == 0) {
    return send_file(req, filepath);
  }

  strlcat(filepath, ".gz", sizeof(filepath));
  if (stat(filepath, &st) == 0) {
    return send_file(req, filepath);
  }

  strlcpy(filepath, "/spiffs/www/index.html.gz", sizeof(filepath));
  return send_file(req, filepath);
}

esp_err_t httpd_get_JSON(httpd_req_t *req, cJSON** json) {
  char content_type[32] = {0};
  httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
  if (strcmp(content_type, "application/json") == 0) {
      char content[256];
      size_t recv_size = MIN(req->content_len, sizeof(content));
      int ret = httpd_req_recv(req, content, recv_size);
      if (ret <= 0) {  /* 0 return value indicates connection closed */
          Serial.println("Failed to recv data");
          return ESP_FAIL;
      } else {
        *json = cJSON_Parse(content);
        if (json == NULL) {
          Serial.println("Failed to parse json");
          return ESP_FAIL;
        }
        return ESP_OK;
      }
  }
  return ESP_FAIL;
}

static esp_err_t pump_config_post_handler(httpd_req_t *req)
{
  // use httpd_req_get_hdr_value_str(httpd_req_t *r, const char *field, char *val, size_t val_size) get content type if json
  cJSON* json = nullptr;
  if (httpd_get_JSON(req, &json) == ESP_OK) {
    Serial.println(cJSON_Print(json));

    pump->setPumpConfig(json);
    
    cJSON_free(json);
    return send_file(req, "/spiffs/pump_config.json");
  }
  cJSON_free(json);
  httpd_resp_send_500(req);
  return ESP_FAIL;
}


// static esp_err_t register_uri_handler(httpd_handle_t server, const char* uri, httpd_method_t method, esp_err_t handler)
// static esp_err_t handler_wrapper(httpd_req_t *req, esp_err_t (*handler)(httpd_req_t *req)) {
//   // *handler(req);
// }

static esp_err_t register_uri_handler(httpd_handle_t server, const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *req))
{
  auto handler_wrapper = [] (httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    esp_err_t (*handler)(httpd_req_t *) = (esp_err_t (*)(httpd_req_t *))req->user_ctx;
    return handler(req);
  };
  httpd_uri_t new_uri = {
    .uri = uri,
    .method = method,
    .handler = handler_wrapper,
    .user_ctx = (void *)handler
  };
  return httpd_register_uri_handler(server, &new_uri);
}

// implement class, that takes freertos queue. It must register consumers callbacks. On queue recive it must send it to callbacks
QueueHandle_t queue;
struct item_t {
  int values[10];
  int len;
} item_t;

class QueueConsumer {
public:
  QueueConsumer(QueueHandle_t queue) : queue_(queue) {}

  void registerConsumer(void (*consumer)(void *)) {
    consumers_.push_back(consumer);
  }

  void start() {
    xTaskCreate(task, "queueConsumer", 2048, this, 1, &task_handle_);
  }

  void stop() {
    vTaskDelete(task_handle_);
  }

private:
  static void task(void *arg) {
    QueueConsumer *self = (QueueConsumer *)arg;

    void* item;
    while (xQueueReceive(self->queue_, &item, portMAX_DELAY) == pdPASS) {
      // cast to item_t
      item_t *item_ptr = (item_t *)item;
      Serial.printf("received item: %d\n", item_ptr->len);
      for (auto consumer : self->consumers_) {
        consumer(item);
      }
    }
    vTaskDelete(NULL);
  }

  QueueHandle_t queue_;
  std::vector<void (*)(void *)> consumers_;
  TaskHandle_t task_handle_;
};




static void testQueueProducerTask(void *pvParameters) {
  item_t item = {
    .values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
    .len = 10
  };

  for(;;) {
    Serial.printf("sending item: %d\n", item.len);
    xQueueSend(queue, &item, portMAX_DELAY);
    Serial.printf("sent item: %d\n", item.len);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // xQueueSend(queue, &queue, portMAX_DELAY);
  }
  vTaskDelete(NULL);
}
static void testQueueConsumer() {
  queue = xQueueCreate(2, sizeof(&item_t));
  QueueConsumer *consumer = new QueueConsumer(queue);
  // consumer->registerConsumer([] (void *item) {
  //   Serial.printf("got item: %d\n", (int)item);
  // });
  // consumer->registerConsumer([] (void *item) {
  //   // cast item to item_t
  //   item_t *item_ptr = (item_t *)item;
  //   Serial.printf("got!!!! item: %d\n", 111);
  // });
  consumer->start();
  xTaskCreate(testQueueProducerTask, "testQueueConsumer", 2048, NULL, 1, NULL);
}

static httpd_handle_t start_webserver()
{
  testQueueConsumer();
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.lru_purge_enable = true;


  if (httpd_start(&server, &config) == ESP_OK)
  {
    register_uri_handler(server, "/pump_config", HTTP_GET, [](httpd_req_t *req) {
      send_file(req, "/spiffs/pump_config.json");
      return ESP_OK;
    });
    
    register_uri_handler(server, "/pump_config", HTTP_POST, pump_config_post_handler);

    register_uri_handler(server, "/*", HTTP_OPTIONS, [](httpd_req_t *req) {
      httpd_resp_set_status(req, "200 OK");
      httpd_resp_send(req, NULL, 0);
      return ESP_OK;
    });

    register_uri_handler(server, "/*", HTTP_GET, rest_common_get_handler);
    
    Serial.printf("HTTP server started at port %d\n", config.server_port);
    return server;
  }

  return NULL;
}

