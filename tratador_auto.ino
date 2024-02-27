#include <Wire.h>
#include <stdint.h>

#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

#include <SD.h>
#include <SPI.h>

// pinos do sc-card (10-cs, 11-mosi, 12-miso, 13-sck) +vcc e GND.
// pinos do LCD (SCL e SDA) +vcc e GND
// Pinos do joystick (7-sw, A0-vry, A1-vrx) +vcc e GND
// Pinos do DS3231 (A4-SDA, A5-SCL) +vcc e GND

//Variáveis de PRE-configuração
#define TEMPO_MOTOR_LIG 20    // Tempo do Motor ligado por grama de ração
#define TEMPERATURA_TRATAR 15 // Em Grau Celsius
#define VALOR_RACAO_PEIXE_DIA 0.01

// Pinos do Display LCD(16x4 I2C)
#define Display_col 20  // Serve para definir o numero de colunas do display utilizado
#define Display_lin 4   // Serve para definir o numero de linhas do display utilizado
#define ende 0x27       // Serve para definir o endereço do display.

//Componentes pinos
#define pinoSS 10
#define buzzer 5
#define rele1 4
#define rele2 6
#define vamosTratar 3

void tom(char pino, int frequencia, int duracao);

// Pinos do Joystick
#define joyStickVX A1
#define joyStickVY A0
#define botao 7

#define valorAnalogico 648  //Pode mudar para 1024 dependendo do tipo joystick

#define arquivoDoSistema "work.txt" // Nome do arquivo

//#define SERIAL_MODE // utilizado como DEBUG para verficar se todos os componentes do hardware estão funcionando

//Funções do joystick

//Mover cursor verticalmente no menu
int8_t movCursorY(uint8_t cursor) {
  return +(analogRead(joyStickVY) >= valorAnalogico && cursor < 3) - (analogRead(joyStickVY) <= 50 && cursor > 0);
}


int8_t movValorY() {
  return -(analogRead(joyStickVY) >= valorAnalogico) + (analogRead(joyStickVY) <= 50);
}
int8_t movValorX() {
  return -(analogRead(joyStickVX) <= valorAnalogico) + (analogRead(joyStickVX) >= 50);
}



bool movDir() {
  return analogRead(joyStickVX) >= valorAnalogico;
}
bool movEsq() {
  return analogRead(joyStickVX) == 0;
}

#define qtdaMaxTratamento 7  // Na real são 8 contando a partir do 0
#define qtdaMinTratamento 2  // e o valor minímo é 2 sendo o tratar inicial e o final

typedef struct {
  uint8_t hora;
  int16_t min;
  uint16_t seg;
} Time;

typedef struct {
  uint8_t dia;
  uint8_t mes;
  uint16_t ano;
} Data_Lote;


// Structs dos MENUS
typedef struct {
  unsigned int *saldo;          //Saldo Ração
  unsigned int *consumoDiario;  //Consumo dia
  unsigned int *diasEstoque;    //Dias de estoque pegar da variável
} Menu_1;

typedef struct {
  Data_Lote *data;           //Ini Lote
  unsigned int *qtdePeixes;  //Qtde Peixes
  unsigned int *diasLote;    //Dias deste Lote
} Menu_2;

typedef struct {
  unsigned int *taxCrescimentoDia;  //Tax cresce dia
  unsigned int *pesTeorico;         //Pes teorico
  unsigned int *racaoPeixeDia;      //Ração peixe dia
} Menu_3;

typedef struct {
  Time *solsticioVer;        //Solstício Verão
  Time *solsticioInv;        //Solstício Inverno
  unsigned int *minRetardo;  //Minutos Retardo
} Menu_4;

//Menu 1
unsigned int variaveisMenu_1[3] = { 0, 0, 0 };  //Puxar varíaveis da opção no menu e colocar numa array
Menu_1 menu_1 = { &variaveisMenu_1[0],&variaveisMenu_1[1], &variaveisMenu_1[2] };

//Menu 2
Data_Lote dataLote;
unsigned int variaveisMenu_2[3] = { 0, 0 };
Menu_2 menu_2 = { &dataLote, &variaveisMenu_2[0], &variaveisMenu_2[1] };

//Menu 3
unsigned int variaveisMenu_3[3] = { 0, 0, 0 };  //Puxar varíaveis da opção no menu e colocar numa array
Menu_3 menu_3 = { &variaveisMenu_3[0], &variaveisMenu_3[1], &variaveisMenu_3[2] };

//Menu 4
unsigned int variaveisMenu_4[3] = { 0, 0, 0 };  //Puxar varíaveis da opção no menu e colocar numa array
Menu_3 menu_4 = { &variaveisMenu_4[0], &variaveisMenu_4[1], &variaveisMenu_4[2] };

Time horaInicialT, horaFinalT;

uint16_t intervaloTrata;
uint16_t tempoPorTratam;
uint8_t qtdaTratar = 7;
uint8_t indexTratar = 7;  //index do horário para tratar o peixe



//Variáveis para o menu
uint8_t cursor = 0;
uint8_t antigoCursorPos = 5;
uint8_t pagAtual = 1;
uint8_t novPag = pagAtual + 1;
bool modoSuspenso = true;
bool atualizarMenu = false;

//Bibliotecas externas
RTC_DS3231 rtc;                                         //Acessar módulo de RTC(acessar data e hora)
LiquidCrystal_I2C lcd(ende, Display_col, Display_lin);  //Acessar o módulo LiquidCrystal(Display) atráves do I2C
DateTime time;
File sistemaFile;

//Funções

//  Menus
void CarregarMenu_1();   // Desenhar textos base do menu
bool AtualizarMenu_1();  // Atualizar e fazer parte lógica do menu

void CarregarMenu_2();
bool AtualizarMenu_2();

void CarregarMenu_3();
bool AtualizarMenu_3();

void CarregarMenu_4();
bool AtualizarMenu_4();

//  Funções para os menus
bool MudarMenu();
void alterarPagina();   //Função que faz alteração das páginas
bool pedirAlteracao();  //Commando que faz pedido de permissão para alterar ás páginas
unsigned int alterarValor(uint8_t pos);
void atualizarHora(Time *tempo, bool primeiraVez);
bool pularPag();

void lerTela(const char *tela, uint8_t start);  //Ler ás telas do SD para o LCD

// Printa no LCD (escrever informaçãos especificas na tela)
void escreverPag();     //Sempre que escreve algo na última linha usar esse comando para rescrever as páginas
void horaPrint(Time *tempo, uint8_t lin);
void escreverVariavel(unsigned int var, uint8_t col, uint8_t row, bool floatFormat);

// Ponteiros para os menus
void (*CarregarMenuAtual)(void);   // Ponteiro para função do menu(lógica) que estiver rodando
bool (*AtualizarMenuAtual)(void);  // Ponteiro para função do menu(lógica) que estiver rodando

void modoAplicacao(); // Modo onde aplicação faz tratamento e fica em modo suspenso

// Funções do sistema
bool horaTratar();
bool tempTratar();

int PerdeuTratar();                                                      // Se passou o horário e não tratou
void ManejarHorarioTratamento(Time *inicio, Time *final, uint8_t qtda);  // Orgainzar/Reoganizar os horários do tratamento

void calcularSistema();
void recalcularSistema();

void (*resetFunc)(void) = 0;  //Resetar o arduino

void SalvarDados();
void CarregarDados();

void setaHorarios(); //Primeira vez que o programa abrir
void atualizarHorarios(Time *horario,int32_t segundos,bool soma); //Atualizar todo dia
int diasDoMes(int month, int year);

// Inicio da execução do SD CARD
void setup() {  // Executado uma vez quando ligado o Arduino
  pinMode(buzzer, OUTPUT);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  digitalWrite(rele1, HIGH);
  digitalWrite(rele2, HIGH);
  pinMode(vamosTratar, INPUT_PULLUP);

  pinMode(pinoSS, OUTPUT);  // Declara pinoSS como saída

  Serial.begin(9600);  // Define BaundRate

#ifdef SERIAL_MODE
  if (SD.begin()) {                              // Inicializa o SD Card
    Serial.println("SD Card pronto para uso.");  // Imprime na tela
  } else {
    Serial.println("Falha na inicialização do SD Card.");
    while (true)
      ;
    return;
  }
#else
  while (!SD.begin())
    ;
#endif

// Incializar RTC
#ifdef SERIAL_MODE
  while (!rtc.begin()) Serial.println("DS3231 não encontrado");
  Serial.println("DS3231 encontrado");
#else
  while (!rtc.begin())
    ;
#endif

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Ajustar o horário

  // Incializar LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.backlight();
  // Inicializar portas dos botões caso não esteja no modo joystick
  pinMode(botao, INPUT_PULLUP);

  //Inicializar o menu incial
  CarregarMenuAtual = &CarregarMenu_1;
  AtualizarMenuAtual = &AtualizarMenu_1;

  CarregarDados();

  setaHorarios();
  ManejarHorarioTratamento(&horaInicialT, &horaFinalT, qtdaTratar);
  Serial.println("teste");
  calcularSistema();
  //recalcularSistema();
}


void loop() {
  /*
Serial.println("Chamando Peixes"); 
for (int beep = 0;beep<5;beep ++){
  if (beep < 5){
   Serial.println(beep); 
   tone(buzzer,900);
   digitalWrite(rele, LOW);
 
   delay(200);
   noTone(buzzer);
     digitalWrite(rele, HIGH);
   delay(200);
  }
  else{
    break;
  }
}
delay(5000);
*/
  (modoSuspenso)?modoAplicacao():Menu();
  //buzinar();
}

void modoAplicacao() {

  lcd.noBacklight();
  lcd.clear();

  while (1) {
    time = rtc.now();

    if(horaTratar() && tempTratar()){
      // código para tratar
    }
    else if(time.hour() == 0 && time.minute() == 0){
      // recalcular os dados e salvar e reniciar o arduino
      
      //SalvarDados();
      resetFunc();
    }
    if(digitalRead(botao) == LOW){
      delay(400);
      if(digitalRead(botao) == LOW){
        modoSuspenso = false;
        break;
      }
    }
  }
}

//###################################################################################
// Parte dos menus
//###################################################################################

void setaHorarios(){

  time = rtc.now();

  uint8_t mesIndex = time.month();
  //uint8_t mesIndex = 12;
  uint8_t mesInicial,somaDias;
  bool soma;

  if (mesIndex <= 6){
    mesInicial = 1;

    horaInicialT = { 5, 25, 0 };
    horaFinalT = { 19, 11, 0 };
    soma=true;
  }
  else{
    mesInicial = 7;

    horaInicialT = { 7, 4, 0 };
    horaFinalT = { 17, 34, 0 };
    soma=false;
  }

  while (mesInicial < mesIndex) {

  #ifdef SERIAL_MODE
    Serial.println(mesInicial);
    Serial.println("dia:"+String(daysInMonth((int)mesInicial,(int)time.year())));
  #endif

    somaDias+=diasDoMes((int)mesInicial,(int)time.year());
    mesInicial++;
  }

  somaDias+=time.day();
  //somaDias+=30;
  #ifdef SERIAL_MODE
    Serial.println(somaDias);
  #endif

  int32_t segundos = (somaDias * 33);

  if(soma){
    atualizarHorarios(&horaInicialT,segundos,true);
    atualizarHorarios(&horaFinalT,segundos,false);
  }else{
    atualizarHorarios(&horaInicialT,segundos,false);
    atualizarHorarios(&horaFinalT,segundos,true);
  }

  return;
}

void atualizarHorarios(Time *horario,int32_t segundos,bool soma){
  if(soma){
    horario->min += segundos / 60;
    horario->seg = segundos % 60;
    horario->hora += horario->min / 60;
    horario->min %= 60;
  }else{
    //Subtrair
    int8_t horas = segundos / 3600; // calcular o número de horas
    segundos %= 3600; // atualizar segundos para o restante após a subtração das horas
    int8_t minutos = segundos / 60; // calcular o número de minutos restantes
    segundos %= 60; // calcular os segundos restantes

    // Subtrair as horas, minutos e segundos calculados dos valores atuais
    horario->hora -= horas;
    horario->min -= minutos;
    horario->seg = segundos;

    // Ajustar as horas e minutos para garantir que estejam dentro dos limites
    while (horario->min < 0) {
      horario->min += 60;
      horario->hora--;
    }
    while (horario->hora < 0) {
      horario->hora += 24;
    }
  }

  return;
}

bool MudarMenu() {

  if (movDir() && pagAtual + 1 <= 5) {
    novPag = pagAtual + 1;
  } else if (movEsq() && pagAtual - 1 != 0) {
    novPag = pagAtual - 1;
  }

  if (digitalRead(botao) == LOW) {
    lcd.setCursor(0, 3);
    lcd.print("Mudando para:" + String(novPag));
    alterarPagina();
    return true;
  }

  return false;
}

// Lógica de alterar as págianas
void alterarPagina() {
  switch (novPag) {
    case 1:
      CarregarMenuAtual = &CarregarMenu_1;
      AtualizarMenuAtual = &AtualizarMenu_1;
      break;
    case 2:
      CarregarMenuAtual = &CarregarMenu_2;
      AtualizarMenuAtual = &AtualizarMenu_2;
      break;
    case 3:
      CarregarMenuAtual = &CarregarMenu_3;
      AtualizarMenuAtual = &AtualizarMenu_3;
      break;
    case 4:
      CarregarMenuAtual = &CarregarMenu_4;
      AtualizarMenuAtual = &AtualizarMenu_4;
      break;
  }

  pagAtual = novPag;
  escreverPag();
  return;
}

//Rescrever a linha 3 indicando a página
void escreverPag() {
  lcd.setCursor(0, 3);
  lcd.print("Pagina Atual:" + String(pagAtual) + " A/P:" + String(novPag));
}

bool pularPag() {
  if (digitalRead(botao) == LOW) {
    delay(500);
    if (digitalRead(botao) != LOW) {
      alterarPagina();
      return true;
    }
  }
  return false;
}

//Perdir alteração de um variável(default = sim)
bool pedirAlteracao() {
  //Se botão tiver precionado
  //delay(5000);
  if (digitalRead(botao) == LOW) {
    lcd.setCursor(0, 3);
    lcd.print("Deseja mudar valor?");
    delay(500);

    bool res = true;
    while (digitalRead(botao) == HIGH) {
      if (movEsq()) res = true;
      else if (movDir()) res = false;

      lcd.setCursor(19, 3);
      lcd.print(res ? "S" : "N");
    }
    escreverPag();
    return res;
  }
  return false;
}

//Lógica de alterar variáveis(apenas do tipo unsigned int)
unsigned int alterarValor(uint8_t pos) {

  char valorTexto[7] = "000000";  // 6 "0" e um "\0"
  uint8_t posCursor = 0;
  bool valorValido = true;

  delay(1000);


  while (digitalRead(botao) == HIGH || valorValido == false) {
    int8_t inputX = movValorX();
    int8_t inputY = movValorY();


    //Printar # no cursor
    lcd.setCursor(pos + posCursor, cursor);
    lcd.print("#");

    if (posCursor + inputX >= 0 && posCursor + inputX < 6) posCursor += inputX;

    if (valorTexto[posCursor] + inputY >= '0' && valorTexto[posCursor] + inputY <= '9') {
      valorTexto[posCursor] += movValorY();
    }

    //Printar o número no cursor
    delay(100);
    lcd.setCursor(pos, cursor);
    lcd.print(valorTexto);
    delay(175);


    char validacao[3]={valorTexto[0],valorTexto[1],valorTexto[2]};
    valorValido = (unsigned int)atoi(validacao) <= 64;

    lcd.setCursor(0, 3);
    if(valorValido == false){
      lcd.print("Valor Invalido");
    }else{
      lcd.print("Valor Valido");
    }
  }
  atualizarMenu = true;

  

  return (unsigned int)atoi(valorTexto);
}

//------------------- LCD --------------------------------//

// Atualiza/Escreve as variáveis onde o cursor estiver em cima
void escreverVariavel(unsigned int var, uint8_t col, uint8_t row, bool floatFormat) {
  lcd.setCursor(col, row);
  for (uint8_t i = col; i < 20; i++) lcd.print(" ");  //Limpar display

  lcd.setCursor(col, row);
  (floatFormat) ? lcd.print(String((float)var / 1000)) : lcd.print(var);
  return;
}

void lerTela(const char *tela, uint8_t start) {
  sistemaFile = SD.open(tela, FILE_READ);

  if (sistemaFile) {
    uint8_t linhas = start;
    lcd.setCursor(0, linhas);
    while (sistemaFile.available()) {  // Exibe o conteúdo do Arquivo
      //Serial.write(myFile.read());

      char c = sistemaFile.read();
      if (c == '\n') {
        linhas++;
        lcd.setCursor(0, linhas);
      } else {
        if (c >= 32) lcd.print((char)c);  //Limpar caracteres sujos
      }
    }

    sistemaFile.close();  // Fecha o Arquivo após ler
  }
}

// -----------------------------------------------------------------------------------------------------------------------------------
//    MENUS
// ------------------------------------------------------------------------------------------------------------------------------------

// ---------------------
//    Primeiro Menu
// ----------------------

// Desenhar textos bases no display
void CarregarMenu_1() {
  lcd.clear();
  lerTela("tela_a_1.txt", 0);


  escreverVariavel(variaveisMenu_1[0], 13, 0, true);
  escreverVariavel(variaveisMenu_1[1], 13, 1, true);
  escreverVariavel(variaveisMenu_1[2], 16, 2, false);
}

// Atualizar(lógica) do primeiro menu
bool AtualizarMenu_1() {
  if(pedirAlteracao() && cursor == 0 && (*menu_1.saldo + 20000 < 30000)){
    *menu_1.saldo += 20000;
    atualizarMenu = true;
  }
  return true;
}

// -------------
// Segundo  Menu
// -------------
void lotePrint() {
  lcd.setCursor(0, 0);
  lcd.print("Dt Lote: " + String(dataLote.dia) + "/" + String(dataLote.mes) + "/" + String(dataLote.ano));
  return;
}


void CarregarMenu_2() {
  lcd.clear();
  lotePrint();
  lerTela("tela_b_2.txt", 1);

  escreverVariavel(variaveisMenu_2[0], 13, 1, false);
  escreverVariavel(variaveisMenu_2[1], 16, 2, false);
}
bool AtualizarMenu_2() {

  if (cursor == 0 && pedirAlteracao()) {
    time = rtc.now();

    dataLote.dia = time.day();
    dataLote.mes = time.month();
    dataLote.ano = 2024;

    atualizarMenu = true;
  }

  else if (cursor == 1 && pedirAlteracao()) {
    variaveisMenu_2[0] = alterarValor(12);
    atualizarMenu = true;
  }

  return true;
}

// -------------
// Terceiro  Menu
// -------------

void CarregarMenu_3() {
  lcd.clear();
  lerTela("tela_c_3.txt", 0);

  escreverVariavel(variaveisMenu_3[0], 12, 0, true);
  escreverVariavel(variaveisMenu_3[1], 12, 1, true);
  escreverVariavel(variaveisMenu_3[2], 16, 2, false);

  lcd.setCursor(17, 0);
  lcd.print("%");

  lcd.setCursor(17, 1);
  lcd.print("Kg");
}

bool AtualizarMenu_3() {
  if (cursor == 0 && pedirAlteracao()) {
    variaveisMenu_3[0] = alterarValor(12);

  } else if (cursor == 1 && pedirAlteracao()) {
    variaveisMenu_3[1] = alterarValor(12);
  }

  return true;
}


void horaPrint(Time *tempo, uint8_t lin) {
  lcd.setCursor(14, lin);
  lcd.print(String(tempo->hora) + ":" + tempo->min);
  return;
}

void CarregarMenu_4() {
  lcd.clear();
  lerTela("tela_d_4.txt", 0);
  horaPrint(&horaInicialT, 0);
  horaPrint(&horaFinalT, 1);
}
bool AtualizarMenu_4() {
  return true;
}

void Menu() {

  //Atualizar pagina da tela
  delay(500);
  lcd.backlight();
  CarregarMenuAtual();
  escreverPag();

  uint8_t contadorParado = 0;
  //Loop do Menu Principal
  while (!modoSuspenso) {
    cursor += movCursorY(cursor);  //Navegar entre as opções

    //Atualizar o cursor no menu
    if (cursor != antigoCursorPos) {

      lcd.setCursor(19, antigoCursorPos);
      lcd.print(" ");
      lcd.setCursor(19, cursor);
      lcd.print("*");
      contadorParado = 0;
    }else{
      if(contadorParado++ >= 252){
        modoSuspenso = true;
        break;
      }
      contadorParado++;
    } 

    #ifdef SERIAL_MODE
      Serial.println(contadorParado);
    #endif
    antigoCursorPos = cursor;
    //Atualizar o menu

    if (cursor == 3) {
      if (MudarMenu() == true) break; // Trocar de tela true = igual continuar na mesma tela e false = trocar de tela(sair do loop e renicializar)
    } else {
      if (pularPag() == true) break;  // Se apertar botao por 1 segundo alterar de tela
      else AtualizarMenuAtual();
    }

    if (atualizarMenu == true) {
      CarregarMenuAtual();
      atualizarMenu = false;
    } else escreverPag();
    delay(200);
  }

  return;
}




//###################################################################################
// Parte da lógica
//###################################################################################

int PerdeuTratar() {
  time = rtc.now();

  uint16_t tempoTotalMinutos = (horaInicialT.hora * 60) + horaInicialT.min;
  tempoTotalMinutos += intervaloTrata * indexTratar;

  uint16_t horaAtualMinutos = time.minute() + (time.hour() * 60);

  //Serial.println(String(tempoTotalMinutos)+"/"+String(horaMinutos));

  //Perdeu alguns minutos mas continua na mesma hora pode ser ainda tratado
  if (horaAtualMinutos <= tempoTotalMinutos + 60) return 3;  //Código para tratar agora

  return 1;
}

//Calcular a diferença em minutos entre as tratadas
void ManejarHorarioTratamento(Time *inicio, Time *final, uint8_t qtda) {

  int tempoTotalMinutos = (horaFinalT.hora - horaInicialT.hora) * 60 + (horaFinalT.min - horaInicialT.min);
  // Calcula a duração de cada tratamento em minutos
  intervaloTrata = tempoTotalMinutos / qtdaTratar;
  return;
}

// Salvar valores das váriaveis na mémoria para o SD
void SalvarDados() {
  sistemaFile = SD.open(arquivoDoSistema, FILE_WRITE);

  if (sistemaFile) {
    SD.remove(arquivoDoSistema);
    sistemaFile = SD.open(arquivoDoSistema, FILE_WRITE);
  }

  sistemaFile.println(String(variaveisMenu_1[0]) + "," + variaveisMenu_1[1] + "," + variaveisMenu_1[2]);
  sistemaFile.println(String(dataLote.dia) + ":" + String(dataLote.mes) + ":" + String(dataLote.ano) + "," + variaveisMenu_2[0] + "," + variaveisMenu_2[1]);
  sistemaFile.println(String(variaveisMenu_3[0]) + "," + variaveisMenu_3[1] + "," + variaveisMenu_3[2]);
  sistemaFile.close();
}

// Carregar valores das váriaveis do SD para a mémoria
void CarregarDados() {
  sistemaFile = SD.open(arquivoDoSistema, FILE_READ);

  if (sistemaFile) {
    char *buffer[3];  //Buffer dos dados
    char *ptr;
    String p;
    uint8_t tela_index = 1;

    // Enquanto há contéudo não-lido no arquivo
    while (sistemaFile.available()) {

      p = sistemaFile.readStringUntil('\n');

      ptr = strtok(p.c_str(), ",");  //Pegar primeiro valor

      // Cada linha pode fornecer apenas 3 valores sendo assim igual no menu (primeira linha = menu_1, segunda linha = menu_2 etc..)
      for (uint8_t index = 0; index < 3; index++) {
        buffer[index] = ptr;
        ptr = strtok(NULL, ",");
      }

      switch (tela_index) {
        case 1:
          for (uint8_t i = 0; i < 3; i++) {
            //Serial.println(buffer[i]);
            variaveisMenu_1[i] = atoi(buffer[i]);
          }
          break;
        case 2:
          for (uint8_t i = 1; i < 3; i++) {
            //Serial.println(buffer[i]);
            variaveisMenu_2[i - 1] = atoi(buffer[i]);
          }


          //Carregar dados da data (utilizando o propio buffer da aplicação)
          ptr = strtok(buffer[0], ":");  //Pegar primeiro valor
          for (uint8_t index = 0; index < 3; index++) {
            buffer[index] = ptr;
            ptr = strtok(NULL, ":");
          }
          dataLote.dia = atoi(buffer[0]);
          dataLote.mes = atoi(buffer[1]);
          dataLote.ano = atoi(buffer[2]);

          break;
        case 3:
          for (uint8_t i = 0; i < 3; i++) {
            variaveisMenu_3[i] = atoi(buffer[i]);
          }
          break;
      }
      tela_index++;
    }
  }
  sistemaFile.close();
  return;
}

void buzinar() {
  if (digitalRead(vamosTratar) == HIGH) {  // só irá tratar manualmente se apertar o botão 3
                                           // para tratar basta setar o pino 3 (vamosTratar) = HIGH
    digitalWrite(vamosTratar,(LOW));
    lcd.noBacklight();
    int tempo = 200;
    for (uint8_t chamarPeixe = 0; chamarPeixe < 3; chamarPeixe++) {
      tom(buzzer, 600, tempo);  //LA
      delay(tempo);
      tom(buzzer, 500, tempo);  //RE
      delay(tempo);
      tom(buzzer, 400, tempo);  //RE
      delay(tempo);
      tom(buzzer, 300, tempo);  //RE
      delay(tempo);
      tom(buzzer, 200, tempo);  //RE
      delay(tempo);
      tom(buzzer, 100, tempo);  //RE
    }
    digitalWrite(rele1, LOW);
    delay(tempo * 8);
    digitalWrite(rele2, LOW);
    delay(tempo * 5);
    digitalWrite(rele1, HIGH);
    digitalWrite(rele2, HIGH);
  }
  Menu();
}

void tom(char pino, int frequencia, int duracao) {
  float periodo = 1000.0 / frequencia;             //Periodo em ms
  for (int i = 0; i < duracao / (periodo); i++) {  //Executa a rotina de dentro o tanta de vezes que a frequencia desejada cabe dentro da duracao
    digitalWrite(pino, HIGH);
    delayMicroseconds(periodo * 500);  //Metade do periodo em ms
    digitalWrite(pino, LOW);
    delayMicroseconds(periodo * 500);
  }
}

//Lógica dos solstícios para encontrar o primeiro e ultimo horário de tratar
// Nascer do sol 01 de janeiro seta = 05:25 soma 00:33 segundos a cada dia 182 vezes (01/julho)
// Por do sol em 01 de Janeiro seta = 19:11 reduz 00:33 segundos a cada dia 182 vezes (01/julho)
//
// Nascer do sol 02 de julho seta = 07:04 reduz 00:33 segundos a cada dia 183 vezes (31/Dezembro)
// Por do sol em 02 de julho seta = 17:34 soma 00:33 segundos a cada dia 183 vezes (31/Dezembro)

int diasDoMes(int month, int year) {
  if (month == 2) { // Fevereiro
    return 28; // Ano não bissexto
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30; // Abril, Junho, Setembro, Novembro
  } else {
    return 31; // Janeiro, Março, Maio, Julho, Agosto, Outubro, Dezembro
  }
}

// Válidar se está na temperatura parar tratar
bool tempTratar(){return (int)rtc.getTemperature() > TEMPERATURA_TRATAR;}

// Válidar se está no horário parar tratar
bool horaTratar(){
  //time = rtc.now(); inicio do loop já faz atualização

  uint16_t horaAtual = (time.hour()*60);
  horaAtual +=time.minute();

  indexTratar=6;
  uint32_t horaTratamento = (horaInicialT.hora*60)+(horaInicialT.min)+indexTratar*intervaloTrata;

  return horaAtual == horaTratamento;
}

void calcularSistema(){
  //*menu_1.saldo = 2000;
  //*menu_3.taxCrescimentoDia = 233;

  Serial.println(*menu_3.taxCrescimentoDia);
  if(*menu_3.taxCrescimentoDia == 0) return;
  


  float taxaCrescimento = *menu_3.taxCrescimentoDia / 100;
  //Serial.println(*menu_1.saldo);

  *menu_3.racaoPeixeDia =(unsigned int) (*menu_3.pesTeorico * taxaCrescimento) * VALOR_RACAO_PEIXE_DIA;
  
  //Serial.println(*menu_3.racaoPeixeDia);

  *menu_1.consumoDiario =(unsigned int) *menu_3.racaoPeixeDia * *menu_2.qtdePeixes;
  *menu_1.diasEstoque =(unsigned int) *menu_1.saldo / *menu_1.consumoDiario;

  //Serial.println(*menu_1.saldo);
  //Serial.println(*menu_1.consumoDiario);

  return;
}

void recalcularSistema(){
  *menu_1.saldo -= *menu_1.consumoDiario;
  *menu_3.pesTeorico = 233;
  
  //Serial.println(*menu_3.pesTeorico);
  *menu_3.pesTeorico += *menu_3.pesTeorico/10;
 // Serial.println(*menu_3.pesTeorico);
}