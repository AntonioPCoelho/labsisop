#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Para a função isspace

/*
 * =======================
 * Documentação da Função main (Versão Completa Atual)
 * =======================
 * Este programa coleta diversas informações do sistema a partir do diretório /proc
 * e as escreve em um arquivo HTML formatado (index.html).
 *
 * Informações coletadas:
 * - Versão do Kernel (/proc/version)
 * - Uptime e Tempo Ocioso (/proc/uptime)
 * - Data e Hora do Sistema (/proc/driver/rtc)
 * - Informações da CPU: Modelo, Velocidade e Núcleos (/proc/cpuinfo)
 */
int main() {
    // --- Variáveis ---
    FILE *arquivo_proc;
    char buffer_leitura[256];
    
    // Variáveis para RTC
    char rtc_date[16] = "N/A";
    char rtc_time[16] = "N/A";
    
    // Variáveis para a CPU
    char cpu_model[128] = "N/A";
    float cpu_mhz = 0.0;
    int cpu_cores = 0;
    int model_found = 0; // Flag para pegar o nome do modelo apenas uma vez
    int mhz_found = 0;   // Flag para a velocidade

    // --- Abertura e Cabeçalho do HTML ---
    FILE *arquivo_html = fopen("index.html", "w");
    if (arquivo_html == NULL) {
        perror("Erro ao criar o arquivo index.html");
        exit(1);
    }
    fprintf(arquivo_html, "<html><head><title>Informacoes do Sistema</title></head><body>\n");
    fprintf(arquivo_html, "<h1>Informacoes do Sistema</h1>\n");

    // --- Coleta da Versão do Kernel ---
    arquivo_proc = fopen("/proc/version", "r");
    if (arquivo_proc != NULL) {
        fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc);
        fprintf(arquivo_html, "<p><b>Versao do sistema e kernel:</b> %s</p>\n", buffer_leitura);
        fclose(arquivo_proc);
    }

    // --- Coleta de Uptime e Tempo Ocioso ---
    arquivo_proc = fopen("/proc/uptime", "r");
    if (arquivo_proc != NULL) {
        double uptime_total_seg, ocioso_total_seg;
        fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc);
        fclose(arquivo_proc);
        sscanf(buffer_leitura, "%lf %lf", &uptime_total_seg, &ocioso_total_seg);
        
        long uptime_long = (long)uptime_total_seg;
        fprintf(arquivo_html, "<p><b>Uptime:</b> %ldd %ldh %ldm %lds</p>\n", uptime_long/86400, (uptime_long%86400)/3600, (uptime_long%3600)/60, uptime_long%60);

        long ocioso_long = (long)ocioso_total_seg;
        fprintf(arquivo_html, "<p><b>Tempo ocioso:</b> %ldd %ldh %ldm %lds</p>\n", ocioso_long/86400, (ocioso_long%86400)/3600, (ocioso_long%3600)/60, ocioso_long%60);
    }

    // --- Coleta de Data e Hora do Sistema via RTC ---
    arquivo_proc = fopen("/proc/driver/rtc", "r");
    if (arquivo_proc != NULL) {
        while (fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc)) {
            if (strstr(buffer_leitura, "rtc_date")) {
                sscanf(buffer_leitura, "rtc_date : %15s", rtc_date);
            } else if (strstr(buffer_leitura, "rtc_time")) {
                sscanf(buffer_leitura, "rtc_time : %15s", rtc_time);
            }
        }
        fclose(arquivo_proc);
    }
    fprintf(arquivo_html, "<p><b>Data do sistema:</b> %s</p>\n", rtc_date);
    fprintf(arquivo_html, "<p><b>Hora do sistema:</b> %s</p>\n", rtc_time);

    // --- Coleta de Informações da CPU ---
    arquivo_proc = fopen("/proc/cpuinfo", "r");
    if (arquivo_proc != NULL) {
        while (fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc)) {
            if (strstr(buffer_leitura, "processor")) {
                cpu_cores++;
            }
            if (strstr(buffer_leitura, "model name") && !model_found) {
                char *colon = strchr(buffer_leitura, ':');
                if (colon != NULL) {
                    char *model_start = colon + 1;
                    while (*model_start && isspace(*model_start)) model_start++;
                    strcpy(cpu_model, model_start);
                    cpu_model[strcspn(cpu_model, "\n")] = 0;
                    model_found = 1;
                }
            }
            if (strstr(buffer_leitura, "cpu MHz") && !mhz_found) {
                sscanf(buffer_leitura, "cpu MHz : %f", &cpu_mhz);
                mhz_found = 1;
            }
        }
        fclose(arquivo_proc);
    }
    fprintf(arquivo_html, "<p><b>Modelo do processador:</b> %s</p>\n", cpu_model);
    fprintf(arquivo_html, "<p><b>Velocidade atual:</b> %.2f MHz</p>\n", cpu_mhz);
    fprintf(arquivo_html, "<p><b>Numero de nucleos:</b> %d</p>\n", cpu_cores);
    
    // --- Rodapé do HTML ---
    fprintf(arquivo_html, "</body>\n</html>\n");
    fclose(arquivo_html);

    printf("Arquivo index.html completo gerado com sucesso!\n");

    return 0;
}
