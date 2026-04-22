# projeto-final-embarcados

       O sistema utiliza sensores para ler temperatura, umidade e luminosidade. Ele possui uma interface visual 
 em um display OLED e permite que o usuário ajuste a temperatura desejada (SetPoint) através de botões físicos.
Além disso, o dispositivo registra as leituras em um histórico armazenado na memória interna do chip.


Utilize a IDE do Arduino ou o simulador Wokwi.

Instale as bibliotecas necessárias: DHTesp, Adafruit_SSD1306 e Adafruit_GFX.

Certifique-se de habilitar o sistema de arquivos LittleFS para o funcionamento do log.


Tela Principal: Mostra as leituras de temperatura, umidade e luz em tempo real.

Menu: Pressione OK para entrar no menu. Use UP/DOWN para navegar.

Ajuste de SetPoint: No menu "SetPoint", use os botões para definir a temperatura ideal da estufa.

Controle Automático: O LED Vermelho acenderá sempre que a temperatura ambiente estiver abaixo do SetPoint definido.


Exemplo de Funcionamento (Wokwi)
Você pode testar o projeto online através deste link:
https://wokwi.com/projects/461888429305547777
