#include "gemini_client.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GEMINI_ENDPOINT "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent"
#define GEMINI_API_KEY  "YOUR_API_KEY_HERE"

/* ============================
   IMMUTABLE SYSTEM PROMPT
   ============================ */

static const char *SYSTEM_PROMPT =
"You are a resume optimization engine.\n"
"You MUST follow these rules:\n"
"- Do not add new skills.\n"
"- Do not invent experience.\n"
"- Only rewrite provided descriptions.\n"
"- Output valid JSON matching the expected schema.\n"
"Any violation is an error.\n";

/* ============================
   HTTP RESPONSE BUFFER
   ============================ */

struct Memory {
    char *data;
    size_t size;
};

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(mem->data + mem->size, contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0';

    return real_size;
}

/* ============================
   PAYLOAD CONSTRUCTION
   ============================ */

static char *build_payload(
    const char *curriculum_json,
    const char *vacancy_json
) {
    const char *template =
        "{"
        "\"contents\": [{"
            "\"role\": \"user\","
            "\"parts\": [{"
                "\"text\": \"%s\\n\\nCURRICULUM:\\n%s\\n\\nVACANCY:\\n%s\""
            "}]"
        "}]}";

    size_t size = strlen(template)
                + strlen(SYSTEM_PROMPT)
                + strlen(curriculum_json)
                + strlen(vacancy_json)
                + 64;

    char *payload = malloc(size);
    snprintf(
        payload,
        size,
        template,
        SYSTEM_PROMPT,
        curriculum_json,
        vacancy_json
    );

    return payload;
}

/* ============================
   GEMINI CALL
   ============================ */

char *gemini_optimize(
    const char *curriculum_json,
    const char *vacancy_json
) {
    CURL *curl = curl_easy_init();
    struct Memory response = { malloc(1), 0 };

    char *payload = build_payload(curriculum_json, vacancy_json);
    char auth_header[256];

    snprintf(
        auth_header,
        sizeof(auth_header),
        "Authorization: Bearer %s",
        GEMINI_API_KEY
    );

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, GEMINI_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(payload);

    if (res != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    return response.data; /* untrusted JSON */
}

char *raw_json = gemini_optimize(curriculum_json, vacancy_json);

if (!raw_json) {
    // fail closed
}

OptimizedResult result = parse_and_validate(raw_json);

free(raw_json);
