#include <stdio.h>
#include "vacancy_fetcher.h"
#include "curriculum_parser.h"
#include "optimizer.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <curriculum.html> <job_url>\n", argv[0]);
        return 1;
    }

    Curriculum curriculum = parse_curriculum(argv[1]);
    Vacancy vacancy = fetch_and_parse_vacancy(argv[2]);

    OptimizedResult result = optimize_curriculum(curriculum, vacancy);

    write_optimized_html("curriculum/optimized.html", curriculum, result);

    printf("Curriculum optimized successfully.\n");
    return 0;
}
