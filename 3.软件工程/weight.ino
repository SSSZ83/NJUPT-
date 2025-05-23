#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// HX711 引脚定义
#define HX711_SCK  13    // GPIO14 (D5)
#define HX711_DT   12    // GPIO12 (D6)

#define GapValue 400

const int potPin = A0;  // 定义模拟输入引脚 A0
int potValue = 0;       // 存储电位器读数的变量
long HX711_Buffer = 0;
long Weight_Maopi = 0, Weight_Shiwu = 0;
long e_weight=0;
int flag=0;
const int BUTTON_COUNT = 2;
const int buttonPins[BUTTON_COUNT] = {4, 5};  // D2, D5 on NodeMCU
volatile bool buttonPressed[BUTTON_COUNT] = {false, false};
volatile unsigned long lastDebounceTime[BUTTON_COUNT] = {0, 0};
long history[10]={0};
int j=0;
unsigned long debounceDelay = 50;  // 防抖动延迟时间（毫秒）
void ICACHE_RAM_ATTR handleInterrupt0() { handleInterrupt(0); }
void ICACHE_RAM_ATTR handleInterrupt1() { handleInterrupt(1); }

void handleInterrupt(int buttonIndex) {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime[buttonIndex] > debounceDelay) {
    buttonPressed[buttonIndex] = true;
    lastDebounceTime[buttonIndex] = currentTime;
  }
}

void Init_Hx711()
{
    pinMode(HX711_SCK, OUTPUT);    
    pinMode(HX711_DT, INPUT);
}

void Get_Maopi()
{
    Weight_Maopi = HX711_Read();        
} 

long Get_Weight()
{
    HX711_Buffer = HX711_Read();
    Weight_Shiwu = HX711_Buffer;
    Weight_Shiwu = Weight_Shiwu - Weight_Maopi;                //获取实物的AD采样数值。
    Weight_Shiwu = (long)((float)Weight_Shiwu/GapValue);     
    return Weight_Shiwu;
}

unsigned long HX711_Read(void)    //增益128
{
    unsigned long count; 
    unsigned char i;

    digitalWrite(HX711_DT, HIGH);
    delayMicroseconds(1);

    digitalWrite(HX711_SCK, LOW);
    delayMicroseconds(1);

    count=0; 
    while(digitalRead(HX711_DT)); 
    for(i=0;i<24;i++)
    { 
        digitalWrite(HX711_SCK, HIGH); 
        delayMicroseconds(1);
        count=count<<1; 
        digitalWrite(HX711_SCK, LOW); 
        delayMicroseconds(1);
        if(digitalRead(HX711_DT))
            count++; 
    } 
    digitalWrite(HX711_SCK, HIGH); 
    count ^= 0x800000;
    delayMicroseconds(1);
    digitalWrite(HX711_SCK, LOW); 
    delayMicroseconds(1);
    
    return(count);
}

void setup() {
    Serial.begin(115200);
    
    // 初始化 OLED
    Wire.begin(2, 14);  // SDA = GPIO4 (D2), SCL = GPIO5 (D1)
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextColor(WHITE);

    // 初始化 HX711
    Init_Hx711();
    Get_Maopi();  // 获取毛皮重量

    Serial.println("Setup completed");

    for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    }
    attachInterrupt(digitalPinToInterrupt(buttonPins[0]), handleInterrupt0, FALLING);
    attachInterrupt(digitalPinToInterrupt(buttonPins[1]), handleInterrupt1, FALLING);

}

void loop() {
  
  potValue = analogRead(potPin);
  int percentage = map(potValue, 0, 1023, 0, 100);
  if(percentage<5){
    long weight = Get_Weight();  // 获取重量
    for (int i = 0; i < BUTTON_COUNT; i++) {
      if (buttonPressed[i]) {
        Serial.print("按键 ");
        Serial.print(i + 1);
        Serial.println(" 被按下！");
        if(i==0){
        flag+=1;
        if(flag%2==1){
          e_weight=weight;
          display.setTextSize(2);
          display.setCursor(50,0);
          display.println("*******");
        }
        else{
          e_weight=0;
        }//去皮功能实现
        }
        if(i==1){
          if(flag%2==1){
            weight-=e_weight;
          }
          history[j]=weight;
          j+=1;
          if(j>9){
            for(int k=0;k<9;k++){
              history[k]=history[k+1];
              history[9]=weight;
              j=9;
            }
          }
        }
        buttonPressed[i] = false;
      }
    }
    // 显示重量
    if(flag%2==1){
       weight-=e_weight;
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Weight:");
    display.setTextSize(2);
    display.setCursor(0,20);
    display.print(weight);
    display.println(" g");
    if(flag%2==1){
      display.setTextSize(1);
      display.setCursor(0,50);
      display.println("*******");
      }
    display.display();
  }
  else{
    int history_case=(percentage-5)/10;
    if(history_case<9){
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Hisrory:");
    display.setTextSize(2);
    display.setCursor(0,20);
    display.print(history_case+1);
    display.println(".");
    display.setCursor(60,20);
    display.print(history[history_case]);
    display.println(" g");
    display.setCursor(0,40);
    display.print(history_case+2);
    display.println(".");
    display.setCursor(60,40);
    display.print(history[history_case+1]);
    display.println(" g");
    display.display();
    }
  }
    delay(200);  // 每秒更新五次
}