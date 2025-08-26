#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>     // Para a função sleep()
#include <sys/file.h>   // <<< MUDANÇA: Header necessário para flock()

#define UPDATE_INTERVAL 5 // <<< MUDANÇA: Intervalo de atualização em segundos

/*
 * =======================
 * Documentação da Função main (Versão para Serviço Contínuo)
 * =======================
 * Este programa roda em um loop contínuo para coletar diversas informações 
 * do sistema a partir do diretório /proc e as escreve em um arquivo HTML 
 * formatado (index.html).
 *
 * Ele utiliza flock() para garantir que a escrita no arquivo seja atômica
 * e não entre em conflito com o servidor web que lê este arquivo.
 */
int main() {

    // <<< MUDANÇA: Início do loop infinito
    while(1) {
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
        int model_found = 0; 
        int mhz_found = 0;   

        // --- Abertura e Travamento do HTML ---
        FILE *arquivo_html = fopen("index.html", "w");
        if (arquivo_html == NULL) {
            perror("Erro ao criar o arquivo index.html");
            sleep(UPDATE_INTERVAL); // Aguarda antes de tentar de novo
            continue; // Pula para a próxima iteração do loop
        }

        // <<< MUDANÇA: Tenta obter uma trava exclusiva no arquivo
        printf("Tentando obter trava exclusiva para escrita...\n");
        if (flock(fileno(arquivo_html), LOCK_EX) != 0) {
            perror("flock (escritor)");
            fclose(arquivo_html);
            sleep(UPDATE_INTERVAL);
            continue;
        }
        printf("Trava obtida. Atualizando index.html...\n");

        // --- Geração do Conteúdo HTML ---
        fprintf(arquivo_html, "<html><head><title>Informacoes do Sistema</title>"
                              // <<< MUDANÇA: Adiciona auto-refresh na página
                              "<meta http-equiv='refresh' content='%d'></head>"
                              "<body>\n", UPDATE_INTERVAL);
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
        // (Este arquivo pode não existir ou não ser legível em todos os sistemas)
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

        // --- Coleta de Uso da CPU ---
        long long prev_user, prev_nice, prev_system, prev_idle, prev_iowait, prev_irq, prev_softirq;
        long long curr_user, curr_nice, curr_system, curr_idle, curr_iowait, curr_irq, curr_softirq;
        long long prev_total_idle, curr_total_idle, prev_total, curr_total;
        double cpu_usage;

        arquivo_proc = fopen("/proc/stat", "r");
        if (arquivo_proc != NULL) {
            fscanf(arquivo_proc, "cpu %lld %lld %lld %lld %lld %lld %lld",
                   &prev_user, &prev_nice, &prev_system, &prev_idle, &prev_iowait, &prev_irq, &prev_softirq);
            fclose(arquivo_proc);
            
            sleep(1); // Pausa de 1 segundo para o cálculo
            
            arquivo_proc = fopen("/proc/stat", "r");
            if (arquivo_proc != NULL) {
                fscanf(arquivo_proc, "cpu %lld %lld %lld %lld %lld %lld %lld",
                       &curr_user, &curr_nice, &curr_system, &curr_idle, &curr_iowait, &curr_irq, &curr_softirq);
                fclose(arquivo_proc);
                
                prev_total_idle = prev_idle + prev_iowait;
                curr_total_idle = curr_idle + curr_iowait;

                prev_total = prev_user + prev_nice + prev_system + prev_idle + prev_iowait + prev_irq + prev_softirq;
                curr_total = curr_user + curr_nice + curr_system + curr_idle + curr_iowait + curr_irq + curr_softirq;

                long long total_diff = curr_total - prev_total;
                long long idle_diff = curr_total_idle - prev_total_idle;
                
                if (total_diff > 0) {
                    cpu_usage = (double)(total_diff - idle_diff) * 100.0 / total_diff;
                } else {
                    cpu_usage = 0.0;
                }
                
                fprintf(arquivo_html, "<p><b>Uso do processador:</b> %.2f%%</p>\n", cpu_usage);
            } else {
                fprintf(arquivo_html, "<p><b>Uso do processador:</b> N/A (Erro na 2a leitura)</p>\n");
            }
        } else {
            fprintf(arquivo_html, "<p><b>Uso do processador:</b> N/A (Erro na 1a leitura)</p>\n");
        }

        // --- Rodapé e Fechamento do Arquivo ---
        fprintf(arquivo_html, "</body>\n</html>\n");
        
        // <<< MUDANÇA: Libera a trava do arquivo
        flock(fileno(arquivo_html), LOCK_UN);
        
        fclose(arquivo_html);

        printf("Arquivo index.html atualizado com sucesso!\n");

        // <<< MUDANÇA: Aguarda o intervalo definido antes de recomeçar o loop
        printf("Aguardando %d segundos...\n\n", UPDATE_INTERVAL);
        sleep(UPDATE_INTERVAL);

    } // <<< MUDANÇA: Fim do loop infinito

    return 0; // Este ponto nunca será alcançado
}
