OptimizedResult optimize_curriculum(Curriculum c, Vacancy v) {
    char *curriculum_json = serialize_curriculum(c);
    char *vacancy_json = serialize_vacancy(v);

    char *optimized_json = gemini_optimize(
        curriculum_json,
        vacancy_json
    );

    return parse_optimized_result(optimized_json);
}

int keyword_score(const char *skill, Vacancy v) {
    int score = 0;
    if (string_contains(v.required_skills, skill)) score += 3;
    if (string_contains(v.responsibilities, skill)) score += 2;
    return score;
}

