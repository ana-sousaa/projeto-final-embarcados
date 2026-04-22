# projeto-final-embarcados

###Descricao do Projeto
O sistema utiliza sensores para ler temperatura, umidade e luminosidade. Ele possui uma interface visual em um display OLED e permite que o usuario ajuste a temperatura desejada (SetPoint) atraves de botoes fisicos. Alem disso, o dispositivo registra as leituras em um historico armazenado na memoria interna do chip.

###Diagrama de Blocos
Configuracao e Instalacao
Ambiente: Utilize a IDE do Arduino ou o simulador Wokwi.

Bibliotecas: Instale as bibliotecas DHTesp, Adafruit_SSD1306 e Adafruit_GFX.

Memoria: Certifique-se de habilitar o sistema de arquivos LittleFS para o funcionamento do log.

###Instrucoes de Uso
Tela Principal: Mostra as leituras de temperatura, umidade e luz em tempo real.

Menu: Pressione OK para entrar no menu. Use UP/DOWN para navegar.

Ajuste de SetPoint: No menu SetPoint, use os botoes para definir a temperatura ideal da estufa.

Controle Automatico: O LED Vermelho acendera sempre que a temperatura ambiente estiver abaixo do SetPoint definido.

###Exemplo de Funcionamento (Wokwi)
Voce pode testar o projeto online atraves do link abaixo:
Projeto no Wokwi
