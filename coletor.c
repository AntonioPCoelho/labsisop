#include <stdio.h>
#include <stdlib.h>

/*
 * =======================
 * Documentação da Função main (Versão 3)
 * =======================
 * Este programa coleta a versão do kernel e o uptime do sistema.
 * - O uptime, lido em segundos de /proc/uptime, é convertido para um formato
 * de dias, horas, minutos e segundos.
 * - As informações são escritas de forma sequencial no arquivo index.html,
 * sem que uma sobrescreva a outra.
 */
int main() {
    // --- Variáveis reutilizáveis ---
    FILE *arquivo_proc;
    char buffer_leitura[256];
    
    // --- PASSO 1: Abrir o arquivo HTML uma única vez ---
    FILE *arquivo_html = fopen("index.html", "w");
    if (arquivo_html == NULL) {
        perror("Erro ao criar o arquivo index.html");
        exit(1);
    }
    
    // --- PASSO 2: Escrever o cabeçalho do HTML ---
    fprintf(arquivo_html, "<html>\n");
    fprintf(arquivo_html, "<head>\n<title>Informacoes do Sistema</title>\n</head>\n");
    fprintf(arquivo_html, "<body>\n<h1>Informacoes do Sistema</h1>\n");

    // --- PASSO 3: Coletar e escrever a Versão do Kernel ---
    arquivo_proc = fopen("/proc/version", "r");
    if (arquivo_proc != NULL) {
        fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc);
        fprintf(arquivo_html, "<p><b>Versao do sistema e kernel:</b> %s</p>\n", buffer_leitura);
        fclose(arquivo_proc); // Tarefa concluída, fecha o arquivo proc.
    }

    // --- PASSO 4: Coletar, converter e escrever o Uptime do Sistema ---
    arquivo_proc = fopen("/proc/uptime", "r");
    if (arquivo_proc != NULL) {
        double uptime_total_seg;
        
        // Lê a linha do arquivo (ex: "12345.67 ...")
        fgets(buffer_leitura, sizeof(buffer_leitura), arquivo_proc);
        fclose(arquivo_proc); // Já lemos, podemos fechar.

        // Extrai apenas o primeiro número (total de segundos) da string lida
        sscanf(buffer_leitura, "%lf", &uptime_total_seg);
        
        // --- Lógica de conversão ---
        long uptime_long = (long)uptime_total_seg; // Converte para inteiro para facilitar os cálculos
        
        int dias = uptime_long / 86400;         // 86400 segundos em um dia
        int horas = (uptime_long % 86400) / 3600; // Resto dos dias, dividido por 3600s/hora
        int minutos = (uptime_long % 3600) / 60;   // Resto das horas, dividido por 60s/min
        int segundos = uptime_long % 60;           // Resto dos minutos

        // Escreve a informação já formatada no HTML
        fprintf(arquivo_html, "<p><b>Uptime:</b> %d dias, %d horas, %d minutos e %d segundos</p>\n", dias, horas, minutos, segundos);
    }

    // --- PASSO 5: Escrever o rodapé do HTML ---
    fprintf(arquivo_html, "</body>\n</html>\n");
    
    // --- PASSO 6: Fechar o arquivo HTML ---
    // Apenas aqui, no final de tudo, nós fechamos o arquivo de saída.
    fclose(arquivo_html);

    printf("Arquivo index.html atualizado com Uptime!\n");

    return 0;
}
