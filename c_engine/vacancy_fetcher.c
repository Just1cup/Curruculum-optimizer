#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vacancy_fetcher.h"

/* ============================
   1.1 – HTTP FETCH
   ============================ */

struct Memory {
    char *data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0';

    return real_size;
}

static char *fetch_html(const char *url) {
    CURL *curl = curl_easy_init();
    struct Memory chunk = { malloc(1), 0 };

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "resume-enhancer/1.0");

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return chunk.data;
}

/* ============================
   1.3 – TEXT EXTRACTION
   (HTML → clean visible text)
   ============================ */

static char *strip_tags(const char *html) {
    size_t len = strlen(html);
    char *out = malloc(len + 1);
    size_t j = 0;
    int in_tag = 0;

    for (size_t i = 0; i < len; i++) {
        if (html[i] == '<') {
            in_tag = 1;
            continue;
        }
        if (html[i] == '>') {
            in_tag = 0;
            continue;
        }
        if (!in_tag) {
            out[j++] = html[i];
        }
    }
    out[j] = '\0';
    return out;
}

static char *normalize_whitespace(char *text) {
    char *src = text;
    char *dst = text;
    int space = 0;

    while (*src) {
        if (isspace((unsigned char)*src)) {
            if (!space) {
                *dst++ = ' ';
                space = 1;
            }
        } else {
            *dst++ = *src;
            space = 0;
        }
        src++;
    }
    *dst = '\0';
    return text;
}

static char *extract_job_text(const char *html) {
    /*
      NOTE:
      This is intentionally conservative.
      Later you can replace this with Gumbo/libxml2
      without changing any external interface.
    */

    char *no_tags = strip_tags(html);
    normalize_whitespace(no_tags);
    return no_tags;
}

/* ============================
   1.4 – STRUCTURED VACANCY PARSING
   (Text → Vacancy struct)
   ============================ */

static int is_section_header(const char *line, const char *keyword) {
    return strncasecmp(line, keyword, strlen(keyword)) == 0;
}

static Vacancy parse_vacancy_text(const char *text) {
    Vacancy v = vacancy_init();

    char *copy = strdup(text);
    char *line = strtok(copy, "\n");

    enum {
        NONE,
        SKILLS,
        RESPONSIBILITIES,
        QUALIFICATIONS
    } section = NONE;

    while (line) {
        if (is_section_header(line, "requirements") ||
            is_section_header(line, "skills")) {
            section = SKILLS;
        } else if (is_section_header(line, "responsibilities")) {
            section = RESPONSIBILITIES;
        } else if (is_section_header(line, "qualifications")) {
            section = QUALIFICATIONS;
        } else {
            if (section == SKILLS) {
                vacancy_add_skill(&v, line);
            } else if (section == RESPONSIBILITIES) {
                vacancy_add_responsibility(&v, line);
            } else if (section == QUALIFICATIONS) {
                vacancy_add_qualification(&v, line);
            }
        }
        line = strtok(NULL, "\n");
    }

    free(copy);
    return v;
}

Vacancy fetch_and_parse_vacancy(const char *url) {
    char *html = fetch_html(url);
    char *text = extract_job_text(html);
    Vacancy vacancy = parse_vacancy_text(text);

    free(html);
    free(text);

    return vacancy;
}
