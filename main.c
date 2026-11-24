#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>

static size_t rec_len = 0;
static size_t key_start = 0, key_end = 0;
static int sort_ascending = 1;

int cmp(const void *a, const void *b) {
    const unsigned char *pa = a, *pb = b;
    uint64_t ka = 0, kb = 0;
    for (size_t i = 0; i <= key_end - key_start; i++) {
        ka |= (uint64_t)pa[key_start + i] << (i * 8);
        kb |= (uint64_t)pb[key_start + i] << (i * 8);
    }
    if (ka < kb) return sort_ascending ? -1 : 1;
    if (ka > kb) return sort_ascending ? 1 : -1;
    return 0;
}

void bubble_sort(void *base, size_t n, size_t size) {
    unsigned char *arr = (unsigned char*)base;
    for (size_t i = 0; i < n - 1; i++)
        for (size_t j = 0; j < n - 1 - i; j++) {
            unsigned char *a = arr + j * size;
            unsigned char *b = arr + (j + 1) * size;
            if (cmp(a, b) > 0)
                for (size_t k = 0; k < size; k++) {
                    unsigned char t = a[k];
                    a[k] = b[k];
                    b[k] = t;
                }
        }
}

void print_records(const unsigned char *data, size_t n, const char *title) {
    printf("\n--- %s ---\n", title);
    for (size_t i = 0; i < n; i++) {
        const unsigned char *rec = data + i * rec_len;
        uint64_t key = 0;
        for (size_t j = 0; j <= key_end - key_start; j++)
            key |= (uint64_t)rec[key_start + j] << (j * 8);
        printf("Запись %3zu | ключ = %10llu |\n", i + 1, (unsigned long long)key);
    }
}

int main(void) {
    setlocale(LC_ALL, ".UTF-8");
    printf("=== Сортировка записей фиксированной длины ===\n");

    char fname[256];
    printf("Имя исходного файла: ");
    if (!fgets(fname, sizeof(fname), stdin)) return 1;
    fname[strcspn(fname, "\n")] = 0;

    printf("Длина записи (байт): ");
    scanf("%zu", &rec_len);
    printf("Номер первого байта реквизита (1..%zu): ", rec_len);
    scanf("%zu", &key_start); key_start--;
    printf("Номер последнего байта реквизита (%zu..%zu): ", key_start+1, rec_len);
    scanf("%zu", &key_end); key_end--;
    printf("Сортировка (1 - возраст., 0 - убыв.): ");
    scanf("%d", &sort_ascending);
    while (getchar() != '\n');

    if (key_start > key_end || key_end >= rec_len) {
        fprintf(stderr, "Ошибка: некорректные границы реквизита.\n");
        return 1;
    }

    FILE *fin = fopen(fname, "rb");
    if (!fin) { perror("Ошибка открытия файла"); return 1; }

    fseek(fin, 0, SEEK_END);
    long fsize = ftell(fin);
    rewind(fin);

    if (fsize % rec_len != 0)
        printf("Предупреждение: размер файла не кратен длине записи.\n");
    size_t nrec = fsize / rec_len;
    if (nrec == 0) {
        fprintf(stderr, "Нет ни одной полной записи.\n");
        fclose(fin);
        return 1;
    }

    unsigned char *buf = malloc(nrec * rec_len);
    if (!buf) { fprintf(stderr, "Ошибка памяти.\n"); fclose(fin); return 1; }
    if (fread(buf, 1, nrec * rec_len, fin) != nrec * rec_len) {
        fprintf(stderr, "Ошибка чтения файла.\n");
        free(buf); fclose(fin);
        return 1;
    }
    fclose(fin);

    print_records(buf, nrec, "До сортировки");
    bubble_sort(buf, nrec, rec_len);
    print_records(buf, nrec, "После сортировки");

    FILE *fout = fopen("FILE.SRT", "wb");
    if (!fout) { perror("Ошибка создания FILE.SRT"); free(buf); return 1; }
    if (fwrite(buf, 1, nrec * rec_len, fout) != nrec * rec_len) {
        fprintf(stderr, "Ошибка записи.\n");
        fclose(fout); free(buf);
        return 1;
    }
    fclose(fout);
    free(buf);

    printf("\nГотово. Результат записан в FILE.SRT\n");
    return 0;
}