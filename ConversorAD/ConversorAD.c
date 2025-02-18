#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define VR_MAX 4082  // Valor máximo dos valores dos eixos
#define VR_MIN 16    // Valor mínimo dos valores dos eixos
#define VR_MED 2049  // Valor médio dos valores dos eixos

#define VRX_PIN 26      // Direcional Joystick no eixo X
#define VRY_PIN 27      // Direcional Joystick no eixo Y
#define BUTTON_A 5      // Botão A
#define BUTTON_B 6      // Botão B
#define BUTTON_JOY 22   // Botão Joystick
#define LED_G 11        // LED Verde
#define LED_R 13        // LED Vermelho
#define LED_B 12        // LED Azul
#define I2C_SDA 14      // Conexão do display (data)
#define I2C_SCL 15      // Conexão do display (clock)

#define I2C_PORT i2c1 
#define ADDRESS 0x3C    // Endereço

ssd1306_t ssd;          // Inicializa a estrutura do display

const uint16_t PERIODO = 4000;              // Período
const float PWM_DIVISOR = 4.0;              // Divisor de PWM
bool pwm_active = true;                     // Ativa ou desativa o modo PWM
bool animate = false;                       // Controle de animação de movimento
static volatile uint32_t last_time = 0;     // Variável de tempo
static volatile uint8_t border_mode = 0;    // Tipo de borda

static void gpio_irq_handler(uint gpio, uint32_t events);   // Função de interrupção
static void setup();                                        // Configuração padrão
static void ssd_setup();                                    // Configuração Display
static void pwm_setup(uint pwm_pin, uint16_t level);        // Configuração PWM

uint8_t x_convert(uint32_t x_value);    // Conversor de valor para posição x no Display
uint8_t y_convert(uint32_t y_value);    // Conversor de valor para posição y no Display
uint16_t level(uint32_t value);         // Conversor de valor para nível do LED

int main() {

    // Inicializando
    stdio_init_all();
    adc_init();
    setup();
    ssd_setup();
 
    // Definindo as funções de callback para os botões por meio de irq
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);      // Botão A
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);      // Botão B
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);    // Botão Joystik
    
    // Inicializando os valores das variáveis que guardarão a posição anterior para o modo de animação
    
    uint8_t prev_x = 60;    // Posição X
    uint8_t prev_y = 28;    // Posição Y

    // Laço principal
    while (true) {
        // Lendo os valores dos direcionais do Joystick
        adc_select_input(0); 
        uint16_t vrv_value = adc_read(); 
        adc_select_input(1); 
        uint16_t vrh_value = adc_read();

        uint8_t x = x_convert(vrh_value);   // Posição de x baseado no valor lido na componente horizontal
        uint8_t y = y_convert(vrv_value);   // Posição de y baseado no valor lido na componente vertical

        // Mostrar no Serial os valores lidos e as posições que deverão estar no Display
        printf("VRX: %u, VRY: %u\n", vrh_value, vrv_value);
        printf("Position: x: %d y: %d\n", x, y);
        printf("LED Levels: R: %d, G : %d, B: %d\n", level(vrv_value), gpio_get(LED_G), level(vrh_value));

        // Alterando o nível dos LEDs baseados nos valores lidos
        pwm_set_gpio_level(LED_B, level(vrv_value));  // LED Azul
        pwm_set_gpio_level(LED_R, level(vrh_value));  // LED Vermelho

        // Se a animação está ativa e houve mudança de posição significativa
        if (animate && (x != prev_x || y != prev_y)) {

            uint8_t frame_rate = 10;                                    // Quantidade de frames
            float i = prev_x, j = prev_y;                               // Valores iniciais de x e y
            float i_step = ((float)x - (float)prev_x)/frame_rate;       // Mudança de passo de i
            float j_step = ((float)y - (float)prev_y)/frame_rate;       // Mudança de passo de j

            for(uint8_t k = 0; k < frame_rate; k++) {                    // Enquanto tivermos menos que 10 frames
                ssd1306_fill(&ssd, false);                              // Limpando o Display
                ssd1306_border(&ssd, 0, 0, 128, 64, true, border_mode); // Desenhando a borda
                ssd1306_draw_char(&ssd, '*',(uint8_t)i, (uint8_t)j);    // Desenhando o quadrado
                ssd1306_send_data(&ssd);                                // Comunicação com o Display

                i += i_step;                                            // Mudando a posição de x
                j += j_step;                                            // Mudando a posição de y

                sleep_ms(100/frame_rate);                               // Delay para legibilidade do Display
            }
        }
        
        // Para garantir que o quadrado esteja no local correto
        ssd1306_fill(&ssd, false);                                      // Limpando o Display
        ssd1306_border(&ssd, 0, 0, 128, 64, true, border_mode);         // Desenhando a borda
        ssd1306_draw_char(&ssd, '*', x, y);                             // Desenhando o quadrado
        ssd1306_send_data(&ssd);                                        // Comunicação com o Display

        prev_x = x; // Salvando a posição de x para animação
        prev_y = y; // Salvando a posição de y para animação

        sleep_ms(400 + !animate * 100); // ~500 ms de delay para diminuir uso da CPU
    }
    return 0;
}
// Configuração dos componentes
static void setup() {

    // Configurando as conexões I2C
    i2c_init(I2C_PORT, 400 * 1000); // Inicializando
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configurando a função do SDA
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configurando a função do SCL
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line

    // Inicializando Joystick
    adc_gpio_init(VRX_PIN);             // Direcional vertical do Joystick
    adc_gpio_init(VRY_PIN);             // Direcional horizontal do Joystick
    gpio_init(BUTTON_JOY);              // Botão do Joystick
    gpio_set_dir(BUTTON_JOY, GPIO_IN);  // Modo de Entrada
    gpio_pull_up(BUTTON_JOY);           // Pull-up

    // Inicializando Botões
    gpio_init(BUTTON_A);                // Botão A
    gpio_set_dir(BUTTON_A, GPIO_IN);    // Modo de Entrada
    gpio_pull_up(BUTTON_A);             // Pull-up

    gpio_init(BUTTON_B);                // Botão B
    gpio_set_dir(BUTTON_B, GPIO_IN);    // Modo de Entrada
    gpio_pull_up(BUTTON_B);             // Pull-up

    // Inicializando LEDs
    gpio_init(LED_G);               // LED Verde
    gpio_set_dir(LED_G, GPIO_OUT);  // Modo de Saída
    gpio_put(LED_G, false);         // Garantir que esteja desligado

    pwm_setup(LED_R, 0);    // Inicializando o LED Vermelho com nível inicial 0
    pwm_setup(LED_B, 0);    // Inicializando o LED Azul com nível inicial 0
}
// Configuração do Display
static void ssd_setup() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ADDRESS, I2C_PORT);    // Inicializando o Display
    ssd1306_config(&ssd);                                           // Configurando o Display
    ssd1306_send_data(&ssd);                                        // Enviando informação para o Display
    ssd1306_fill(&ssd, false);                                      // Limpando o display
    ssd1306_send_data(&ssd);                                        // Enviando informação para o Display
}
// Configuração dos pinos PWM
static void pwm_setup(uint pwm_pin, uint16_t level) {
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);      // Inicializando como PWM
    uint slice = pwm_gpio_to_slice_num(pwm_pin);    // Obtendo o Slice
    pwm_set_clkdiv(slice, PWM_DIVISOR);             // Definindo o divisor do clock
    pwm_set_wrap(slice, PERIODO);                   // Definindo o valor do WRAP
    pwm_set_gpio_level(pwm_pin, level);             // Colocando o nível inicial no pino
    pwm_set_enabled(slice, pwm_active);             // Ligando o Slice como PWM
}
// Função para converter o valor horizontal do Joystick em uma coordenada no Display
uint8_t x_convert(uint32_t x_value) {
    float x_position;                                       // Variação da posição (Float permite um valor posicional mais preciso)
    if (x_value >= 1850 && x_value <= 2100) {               // Se o valor enviado estiver na "Zona Morta"
        x_position = 60;                                    // Define como sendo no meio
    } else {
        x_position = (128.0/(VR_MAX - VR_MIN)) * x_value;   // Utiliza-se um coeficiente angular para determinar a posição
        if (x_position > 120) {                             // Se ultrapassar o limite de 120
            x_position = 120;                               // Retorna para 120
        } else if (x_position < 0) {                        // Verificação apenas para garantir que não esteja abaixo de 0
            x_position = 0;                                 // Retorna para 0
        }
    }
    return (uint8_t) x_position;                            // Retorna a posição x no display
}
// Função para converter o valor vertical do Joystick em uma coordenada no Display
uint8_t y_convert(uint32_t y_value) {
    float y_position;                                                   // Variação da posição
    if (y_value >= 1850 && y_value <= 2000) {                           // Se o valor enviado estiver na "Zona Morta"
        y_position = 28;                                                // Define como sendo no meio
    } else {
        y_position = ((64.0/(VR_MAX - VR_MIN)) * (VR_MAX - y_value));   // O eixo y no Display é invertido, logo, adapta-se a formula
        if (y_position > 56) {                                          // Se ultrapassar o limite de 56
            y_position = 56;                                            // Retorna para 56
        } else if (y_position < 0) {                                    // Verificação apenas para garantir que não esteja abaixo de 0
            y_position = 0;                                             // Retorna para 0
        }
    }
    return (uint8_t) y_position;                                        // Retorna a posição y no display
}
// Função para interrupção dos botões
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) {                                // 200 ms de debouncing
        last_time = current_time;                                           // Atualiza o tempo do último evento
        switch (gpio) {                                                     // Verifica qual botão foi pressionado

            case BUTTON_A:                                                  // Se for o Botão A
                pwm_active = !pwm_active;                                   // Muda o estado dos pinos PWM
                if (pwm_active)                                             // Mostra no Serial qual o estado atual dos PWMs
                    printf("PWM ENABLED\n");                                 
                else
                    printf("PWM DISABLED\n");
                pwm_set_enabled(pwm_gpio_to_slice_num(LED_B), pwm_active);  // Muda o estado do PWM do LED Vermelho
                pwm_set_enabled(pwm_gpio_to_slice_num(LED_R), pwm_active);  // Muda o estado do PWM do LED Azul
                break;
            
            case BUTTON_B:                                                  // Se for o botão B                                                          
                animate = !animate;                                         // Muda o estado da animação
                if (animate)                                                // Mostra no Serial se a animação está ligada ou não.
                    printf("ANIMATION ENABLED\n");
                else
                    printf("ANIMATION DISABLED\n"); 
                break;

            case BUTTON_JOY:                                                // Se for o botão o Joystick
                gpio_put(LED_G, !gpio_get(LED_G));                          // Altera o estado do LED Verde
                border_mode = (border_mode + 1) % 5;                        // Muda o estilo de borda do display
                break;

            default:                                                        // Default por segurança
                break;
    }
        
    }
}
// Função utilizada para determinar o nível de um LED
uint16_t level(uint32_t value) {
    uint16_t level;                         // Inicializando o valor do nível
    if (value >= 1850 && value <= 2100) {   // Se o valor estiver na "Zona Morta"
        level = 0;                          // O LED deverá estar desligado
    } else {                                // Se não
        if (value > VR_MED){                // Se o valor lido for acima do valor médio
            level = value - VR_MED;         // Nível = Valor - Média
        } else {                            // Se não
            level = (VR_MED - value);       // Nível = Média - Valor
        }                                   // Garantindo que não ocorra Underflow
    }
    return level;                           // Retornando o nível calculado
}