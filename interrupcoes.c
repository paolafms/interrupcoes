#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define TEMPO_ATUALIZACAO 100  // Tempo de atualização dos LEDs RGB (100 ms)
#define TEMPO_PISCA 100        // Tempo de piscar o LED (100 ms ligado, 100 ms desligado)
#define DEBOUNCE_TIME 400       // Tempo de debounce em ms

// Pinos dos botões
const uint botao_A_pin = 5;      // Botão A = 5
const uint botao_B_pin = 6;      // Botão B = 6
const uint botao_Joy_pin = 22;   // Botão do Joystick = 22

// Pinos dos LEDs RGB
const uint led_vermelho = 13;    // Red => GPIO13
const uint led_verde = 11;       // Green => GPIO11
const uint led_azul = 12;        // Blue => GPIO12

// Variável global para armazenar o número atual
int numero = 0;

// Buffer para armazenar quais LEDs estão ligados na matriz 5x5
bool led_buffer[NUM_PIXELS] = {0};

// Variável para controlar se os LEDs estão ligados ou desligados
bool leds_ligados = true;

// Variáveis para debounce
uint32_t ultimo_tempo_A = 0;
uint32_t ultimo_tempo_B = 0;
uint32_t ultimo_tempo_Joy = 0;

// Padrões para os números de 0 a 9
bool numeros[10][NUM_PIXELS] = {
   {0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0}, // 0

   {0, 0, 1, 0, 0,  
    0, 0, 1, 0, 0,  
    0, 0, 1, 0, 0,  
    0, 1, 1, 0, 0,  
    0, 0, 1, 0, 0}, // 1

   {0, 1, 1, 1, 0,  
    0, 1, 0, 0, 0,  
    0, 1, 1, 1, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0}, // 2

   {0, 1, 1, 1, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0}, // 3

   {0, 1, 0, 0, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 0, 1, 0}, // 4

   {0, 1, 1, 1, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 1, 0, 0, 0,  
    0, 1, 1, 1, 0}, // 5

   {0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 1, 0, 0, 0,  
    0, 1, 1, 0, 0}, // 6

   {0, 1, 0, 0, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 0, 0, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0}, // 7

   {0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0}, // 8

   {0, 1, 1, 1, 0,  
    0, 0, 0, 1, 0,  
    0, 1, 1, 1, 0,  
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0} // 9
};

// Função para enviar um pixel para a matriz de LEDs
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função para converter RGB para formato WS2812
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Declaração da função set_one_led
void set_one_led(uint8_t r, uint8_t g, uint8_t b);

// Função para atualizar a matriz de LEDs com o número atual
void atualizar_leds(int numero)
{
    // Copia o padrão do número atual para o buffer de LEDs
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_buffer[i] = numeros[numero][i];
    }
}

// Função para apagar todos os LEDs
void apagar_leds()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_buffer[i] = 0;
    }
    set_one_led(0, 0, 0);  // Define todos os LEDs como desligados
}

// Função para definir a cor de um LED
void set_one_led(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (led_buffer[i] && leds_ligados)
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

// Função de interrupção para os botões
void botao_irq_handler(uint gpio, uint32_t events)
{
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    if (gpio == botao_A_pin && tempo_atual - ultimo_tempo_A > DEBOUNCE_TIME)
    {
        numero = (numero + 1) % 10;  // Incrementa e mantém entre 0 e 9
        atualizar_leds(numero);
        ultimo_tempo_A = tempo_atual;
    }
    else if (gpio == botao_B_pin && tempo_atual - ultimo_tempo_B > DEBOUNCE_TIME)
    {
        numero = (numero - 1 + 10) % 10;  // Decrementa e mantém entre 0 e 9
        atualizar_leds(numero);
        ultimo_tempo_B = tempo_atual;
    }
    else if (gpio == botao_Joy_pin && tempo_atual - ultimo_tempo_Joy > DEBOUNCE_TIME)
    {
        leds_ligados = !leds_ligados;  // Alterna o estado dos LEDs
        if (leds_ligados)
        {
            atualizar_leds(numero);  // Acende os LEDs com o número atual
        }
        else
        {
            apagar_leds();  // Apaga todos os LEDs
        }
        ultimo_tempo_Joy = tempo_atual;
    }
}

int main()
{
    stdio_init_all();

    // Inicializa os botões
    gpio_init(botao_A_pin);
    gpio_set_dir(botao_A_pin, GPIO_IN);
    gpio_pull_up(botao_A_pin);

    gpio_init(botao_B_pin);
    gpio_set_dir(botao_B_pin, GPIO_IN);
    gpio_pull_up(botao_B_pin);

    gpio_init(botao_Joy_pin);
    gpio_set_dir(botao_Joy_pin, GPIO_IN);
    gpio_pull_up(botao_Joy_pin);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(botao_A_pin, GPIO_IRQ_EDGE_FALL, true, &botao_irq_handler);
    gpio_set_irq_enabled_with_callback(botao_B_pin, GPIO_IRQ_EDGE_FALL, true, &botao_irq_handler);
    gpio_set_irq_enabled_with_callback(botao_Joy_pin, GPIO_IRQ_EDGE_FALL, true, &botao_irq_handler);

    // Inicializa os LEDs RGB
    gpio_init(led_vermelho);
    gpio_set_dir(led_vermelho, GPIO_OUT);
    gpio_put(led_vermelho, 0);

    gpio_init(led_verde);
    gpio_set_dir(led_verde, GPIO_OUT);
    gpio_put(led_verde, 0);

    gpio_init(led_azul);
    gpio_set_dir(led_azul, GPIO_OUT);
    gpio_put(led_azul, 0);

    // Inicializa a matriz de LEDs
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Inicializa o número 0
    atualizar_leds(numero);

    // Variáveis para o controle do LED piscante
    uint32_t tempo_anterior_pisca = 0;
    bool estado_pisca = false;

    while (1)
    {
        // LED pisca 5 vezes por segundo
        if (to_ms_since_boot(get_absolute_time()) - tempo_anterior_pisca >= TEMPO_PISCA)
        {
            estado_pisca = !estado_pisca;  // Alterna o estado do LED
            gpio_put(led_vermelho, estado_pisca);
            tempo_anterior_pisca = to_ms_since_boot(get_absolute_time());
        }

        // Atualiza a matriz de LEDs com o número atual
        set_one_led(0, 0, 30);  // Cor: Azul
        sleep_ms(TEMPO_ATUALIZACAO);  // Tempo de atualização dos LEDs RGB
    }

    return 0;
}