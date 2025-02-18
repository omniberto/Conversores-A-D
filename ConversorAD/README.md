# FUNCIONAMENTO
1. Botão A: Muda o estado do PWM dos LEDs Vermelho e Azul.
    1. PWM Desligado: Os LEDs não respondem mais ao movimento do Joystick e permanecem desligados.
    2. PWM Ligado: A intensidade dos LEDs são correlacionadas à posição dos eixos do Joystick.
        1. Eixo Vertical -> LED Azul.
        2. Eixo Horizontal -> LED Vermelho.
2. Componentes Joystick:
    1. Componente Vertical:
        1. Controla a intensidade do LED Azul;
        2. Controla a posição Y no Display.
    2. Componente Horizontal:
        1. Controla a intendidade do LED Vermelho;
        2. Controla a posição X no Display.
    3. Botão Joystick:
        1. Muda a borda no Display
            1. Existem 5 configurações de borda no Display:
                1. Borda Desligada (Padrão);
                2. Borda Fina;
                3. Borda Fina Alternada;
                4. Borda Grossa com Cantos modificados;
                5. Borda Grossa Xadrez.
        2. Muda o estado do LED Verde.
            1. O LED Verde inicia desligado e alterna entre Ligado e Desligado.
3. Display: Demonstra a posição dos componentes do Joystick através de um quadrado 8x8 no Display.
4. Botão B (Feature Extra):
    1. Existe no código, uma rotina que permite que a movimentação periódica no Display (Animação);
    2. O Botão B controlará se a animação está habilitada ou não;
    3. A animação começará desabilitada;
    4. ESSA FEATURE NÃO FAZ PARTE DA ATIVIDADE.