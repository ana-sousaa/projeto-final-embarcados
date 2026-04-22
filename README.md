# projeto-final-embarcados

#   Sistema de Monitoramento e Controle de Estufa

##   Descrição do Projeto
O sistema utiliza sensores para ler temperatura, umidade e luminosidade. Ele possui uma interface visual em um display OLED e permite que o usuário ajuste a temperatura desejada (SetPoint) através de botões físicos. Além disso, o dispositivo registra as leituras em um histórico armazenado na memória interna do chip.

##   Configuração e Instalação
* **Ambiente:** Utilize a IDE do Arduino ou o simulador Wokwi.
* **Bibliotecas:** Instale as bibliotecas `DHTesp`, `Adafruit_SSD1306` e `Adafruit_GFX`.
* **Memória:** Certifique-se de habilitar o sistema de arquivos **LittleFS** para o funcionamento do log.

##   Instruções de Uso
* **Tela Principal:** Mostra as leituras de temperatura, umidade e luz em tempo real.
* **Menu:** Pressione **OK** para entrar no menu. Use **UP/DOWN** para navegar.
* **Ajuste de SetPoint:** No menu "SetPoint", use os botões para definir a temperatura ideal da estufa.
* **Controle Automático:** O LED Vermelho acenderá sempre que a temperatura ambiente estiver abaixo do SetPoint definido.

##   Exemplo de Funcionamento (Wokwi)
Você pode testar o projeto online através do link abaixo:
👉 [Projeto no Wokwi](https://wokwi.com/projects/461888429305547777)
