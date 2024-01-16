ou#include<RTClib.h>
#include<Wire.h>
#include <LiquidCrystal_I2C.h>


//Variáveis de PRE-configuração
#define TEMPO_MOTOR_LIG 20 //Tempo do Motor ligado por grama de ração

//Display variáveis
#define Display_col 20 // Serve para definir o numero de colunas do display utilizado
#define Display_lin  4 // Serve para definir o numero de linhas do display utilizado
#define ende  0x27 // Serve para definir o endereço do display.

//Variáveis de parâmetros
unsigned int qtda_animais;
String tipo_animal;
float peso_animal;
float tax_crescimento; //Taxa de crescimento do Animal por dia

unsigned int racao_diaria,racao_por_refeicao;
unsigned int qtda_racao;

unsigned int qtda_refeicao;//Min 3


//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de hórario em tempo real
LiquidCrystal_I2C lcd_1(ende,Display_col,Display_lin); //Acessar o Display LiquidCrystal atráves do I2C


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  if(! rtc.begin()) {
    Serial.println("DS3231 não encontrado");
    while(1);
  }
  rtc.adjust(DateTime((__DATE__), (__TIME__)));// Ajustar o horário
  if(rtc.lostPower()){
    Serial.println("DS3231 OK!");
  }

  lcd_1.init();
  lcd_1.clear();
  lcd_1.backlight();


}

void loop() {
  
  rtc.adjust(DateTime((__DATE__), (__TIME__)));// Ajustar o horário
  DateTime agora = rtc.now();                             // Faz a leitura de dados de data e hora
  
  lcd_1.setCursor(0, 0);
  lcd_1.print("DIA:"+String(agora.day(), DEC));
  lcd_1.setCursor(0, 1);
  lcd_1.print("MES:"+String(agora.month(), DEC));
  lcd_1.setCursor(0, 2);
  lcd_1.print("ANO:"+String(agora.year(), DEC));
  lcd_1.setCursor(0, 3);
  lcd_1.print("HORA:"+String(agora.hour(), DEC)+":"+String(agora.minute(), DEC));
  

  delay(1000);   
}
