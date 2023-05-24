#include <driver/adc.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>

#include "wav.hpp"

// Referente a configurações do ADC
#define ADC_BUFFER       1024
#define ADC_NUM_BYTES    512 // 256 amostras de 16 bits

// Dupla de buffers. Eles são trocados sempre que há dados suficientes para escrita.
// O tamanho deve ser um múltiplo de ADC_NUM_BYTES / 2
#define EXCHANGE_BUFFER_SIZE 4096
uint8_t buffer1[EXCHANGE_BUFFER_SIZE];
uint8_t buffer2[EXCHANGE_BUFFER_SIZE];
size_t loopCounter = 0;
volatile uint8_t *loopPointer = buffer1;  // usado no loop
volatile uint8_t *storePointer = buffer2; // usado na tarefa de gravação

TaskHandle_t storeTaskHandle;

void swap_buffers() {
    volatile uint8_t *tmp = loopPointer;
    loopPointer = storePointer;
    storePointer = tmp;
}

static void continuous_adc_init(void)
{
    // Inicializa o driver ADC
    adc_digi_init_config_t adc_dma_config = {
        .max_store_buf_size = 1024,
        .conv_num_each_intr = ADC_NUM_BYTES,
        .adc1_chan_mask = BIT(7), // Canal 7 (pino 35 do ESP32)
        .adc2_chan_mask = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

    // Configura o canal 7 do ADC1 (GPIO35)
    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN_DB_11,
        .channel = ADC1_CHANNEL_7,
        .unit = 0,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    };

    // Configura o ADC para operar no modo DMA
    adc_digi_configuration_t dig_cfg = {
        .conv_limit_en = 1, // Segundo a documentação do SDK v4.4.4 esse valor deve ser 1 para ESP32
        .conv_limit_num = 250,
        .pattern_num = 1, // Apenas um canal é utilizado
        .adc_pattern = &adc_pattern,
        .sample_freq_hz = 44100, // Amostragem a 44,1kS/s (áudio a 44,1kHz)
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };
    ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}

Wav8BitLoader *wav = nullptr;
void storeTask(void *param) {
    while(true) {
        uint32_t taskCount = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        wav->writeData((uint8_t *)storePointer, EXCHANGE_BUFFER_SIZE);
    }
}

void setup() {
 
    Serial.begin(115200);

    if(!SD.begin(SS, SPI, 10000000)) {
        Serial.println("Erro ao inicializar SD!");
        ESP.restart();
    }

    wav = new Wav8BitLoader(SD, "/novo.wav");
    if(wav->header.chunkSize == 0) {
        Serial.println("Erro ao ler arquivo!");
        ESP.restart();
    }

    // Cria tarefa de gravação
    xTaskCreate(storeTask, "SD task", EXCHANGE_BUFFER_SIZE, nullptr, 1, &storeTaskHandle);
    if(storeTaskHandle == nullptr) {
        Serial.println("Falha ao criar tarefa");
        ESP.restart();
    }

    continuous_adc_init();

    Serial.println("Iniciando captura de som");
    adc_digi_start();
}

void loop() {
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[ADC_NUM_BYTES];

    // Espera pelos resultados do ADC por até 1 segundo
    ret = adc_digi_read_bytes(result, ADC_NUM_BYTES, &ret_num, 1000);
    if(ret == ESP_OK) {
        for(size_t i = 0; i < ret_num; i += 2) {
            adc_digi_output_data_t *data = reinterpret_cast<adc_digi_output_data_t*>(&result[i]);
            uint8_t value = (data->type1.data >> 4) | ((data->type1.data >> 4) & 0x07);
            loopPointer[loopCounter++] = value;
        }

        // Observe que ADC_NUM_BYTES devem ser lidos por vez de forma que o teste abaixo funcione.
        // Como cada amostra tem 2 bytes temos ADC_NUM_BYTES / 2 (256) amostras por vez.
        // Quando houver 2048 / 2 amostras notifica storeTask para gravar os dados.
        if(loopCounter == EXCHANGE_BUFFER_SIZE) {
            loopCounter = 0;
            // Troca os buffers para poder continuar capturando os dados do ADC
            // enquanto a tarefa storeTask grava os bytes no cartão SD
            swap_buffers();
            // Notifica storeTask que há dados a serem gravados
            xTaskNotifyGive(storeTaskHandle);
        }

    } else if (ret != ESP_ERR_TIMEOUT) {
        Serial.println("Erro inesperado!");
        ESP.restart();
    }
}



