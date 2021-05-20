#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <json-c/json.h>

struct string {
    char *ptr;
    size_t len;
};

typedef struct city_s {
    char *city_name;
    double temperature;
    char *weather_description;
    struct city_s *next;
} city;


void init_string(struct string **s) {
    (*s)->len = 0;
    (*s)->ptr = malloc((*s)->len + 1);
    if ((*s)->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    (*s)->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

int callapi(struct string **s, const char *city) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();

    char *url = strdup("api.openweathermap.org/data/2.5/weather?q=");
    url = realloc(url, strlen(url) + strlen(city) + 1);
    strcat(url, city);

    url = realloc(url, strlen(url) + 61);
    strcat(url, "&units=metric&lang=fr&appid=c9e0dcae17847564446745330365c7d0");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, *s);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 404) {
                fprintf(stderr, "La ville '%s' n'existe pas\n\n", city);
            } else {
                fprintf(stderr, "error when calling url : %s\nCode HTTP: %ld\n\n", curl_easy_strerror(res), http_code);
            }
            return 1;
        }
        curl_easy_cleanup(curl);
    }
    return 0;
}

city *get_city_struct_from_json(const struct string *json) {
    struct json_object *parsed_json = json_tokener_parse(json->ptr);
    struct json_object *city_name;
    json_object_object_get_ex(parsed_json, "name", &city_name);

    struct json_object *main;
    json_object_object_get_ex(parsed_json, "main", &main);

    struct json_object *temp;
    json_object_object_get_ex(main, "temp", &temp);

    struct json_object *weather;
    json_object_object_get_ex(parsed_json, "weather", &weather);

    struct json_object *weather_01 = json_object_array_get_idx(weather, 0);

    struct json_object *description;
    json_object_object_get_ex(weather_01, "description", &description);

    city *new_city = malloc(sizeof(city));
    new_city->city_name = strdup(json_object_get_string(city_name));
    new_city->temperature = json_object_get_double(temp);
    new_city->weather_description = strdup(json_object_get_string(description));
    new_city->next = NULL;
    return new_city;
}

void print_city_struct(const city *city) {
    printf("%s : %.2lf°C\nLe ciel est %s\n\n",
           city->city_name,
           city->temperature,
           city->weather_description);
}

void print_weather(const struct string *s, city **previous) {
    city *new_city = get_city_struct_from_json(s);
    if (*previous) {
        (*previous)->next = new_city;
    } else {
        (*previous) = new_city;
    }

    print_city_struct(new_city);
}

void get_weather(const char *city_json, city **previous) {
    struct string *s = malloc(sizeof(struct string));
    init_string(&s);
    if (!callapi(&s, city_json)) {
        print_weather(s, previous);
    }
}

void get_max_temp_city(city *first_city) {
    city *current = first_city;
    city *max_temp_city = current;
    while (current) {
        if (current->temperature > max_temp_city->temperature) {
            max_temp_city = current;
        }
        current = current->next;
    }

    printf("La ville la plus chaude est :\n");
    print_city_struct(max_temp_city);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Must provide city in arg");
        exit(1);
    }

    city *first_city = NULL;
    city **last_city = &first_city;

    for (int i = 1; i < argc; i++) {
        get_weather(argv[i], last_city);
        if (*last_city && (*last_city)->next) {
            last_city = &((*last_city)->next);
        }
    }

    get_max_temp_city(first_city);

    return 0;
}
