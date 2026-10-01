/*========================================================
 * BAI 3 - ADC STM32F103
 *
 * CPU / PCLK2 = 8 MHz
 * USART1 = 9600 baud
 *
 * ADC1 Channel 3
 * PA3 = Analog Input
 *
 * Bien tro:
 *   Chan ngoai -> 3.3V
 *   Chan giua  -> PA3
 *   Chan ngoai -> GND
 *
 * Khong dung HAL
 *========================================================*/


/*========================================================
 * RCC
 *========================================================*/
#define RCC_APB2ENR   (*(volatile unsigned int *)0x40021018)


/*========================================================
 * GPIOA
 *========================================================*/
#define GPIOA_CRL     (*(volatile unsigned int *)0x40010800)
#define GPIOA_CRH     (*(volatile unsigned int *)0x40010804)


/*========================================================
 * USART1
 *========================================================*/
#define USART1_SR     (*(volatile unsigned int *)0x40013800)
#define USART1_DR     (*(volatile unsigned int *)0x40013804)
#define USART1_BRR    (*(volatile unsigned int *)0x40013808)
#define USART1_CR1    (*(volatile unsigned int *)0x4001380C)


/*========================================================
 * ADC1
 *========================================================*/
#define ADC1_SR       (*(volatile unsigned int *)0x40012400)
#define ADC1_CR1      (*(volatile unsigned int *)0x40012404)
#define ADC1_CR2      (*(volatile unsigned int *)0x40012408)
#define ADC1_SMPR2    (*(volatile unsigned int *)0x40012410)
#define ADC1_SQR1     (*(volatile unsigned int *)0x4001242C)
#define ADC1_SQR3     (*(volatile unsigned int *)0x40012434)
#define ADC1_DR       (*(volatile unsigned int *)0x4001244C)


/*========================================================
 * DELAY
 *
 * CPU = 8 MHz
 * Tạo khoảng trễ tương đối bằng vòng lặp.
 *========================================================*/
void delay_ms(unsigned int ms)
{
    volatile unsigned int i;

    while (ms--)
    {
        for (i = 0; i < 2000; i++)
        {
            __asm volatile ("nop");
        }
    }
}


/*========================================================
 * USART1 INIT
 *
 * PA9  = TX
 * PA10 = RX
 *
 * PCLK2 = 8 MHz
 * Baudrate = 9600
 * BRR = 0x0341
 *========================================================*/
void USART1_Init(void)
{
    /* Bước 1: cấp clock cho GPIOA */
    RCC_APB2ENR |= (1 << 2);

    /* Bước 2: cấp clock cho USART1 */
    RCC_APB2ENR |= (1 << 14);


    /*-----------------------------------------------
     * PA9 = USART1_TX
     * PA10 = USART1_RX
     *
     * PA9/PA10 nằm trong GPIOA_CRH.
     *-----------------------------------------------*/
    GPIOA_CRH &= 0xFFFFF00F;
    GPIOA_CRH |= 0x000004B0;


    /*-----------------------------------------------
     * PCLK2 = 8 MHz
     * Baudrate = 9600
     *
     * USARTDIV = 8MHz / (16 × 9600)
     * BRR = 0x0341
     *-----------------------------------------------*/
    USART1_BRR = 0x0341;


    /*-----------------------------------------------
     * UE = bit 13: bật USART
     * TE = bit 3 : cho phép truyền
     * RE = bit 2 : cho phép nhận
     *-----------------------------------------------*/
    USART1_CR1 =
        (1 << 13) |
        (1 << 3)  |
        (1 << 2);
}


/*========================================================
 * USART1 SEND CHAR
 *========================================================*/
void USART1_SendChar(char c)
{
    /* TXE = bit 7
     * Chờ thanh ghi truyền rỗng */
    while (!(USART1_SR & (1 << 7)))
    {
    }

    /* Ghi dữ liệu cần truyền vào DR */
    USART1_DR = c;
}


/*========================================================
 * USART1 SEND STRING
 *========================================================*/
void USART1_SendString(const char *str)
{
    /* Gửi từng ký tự cho đến '\0' */
    while (*str)
    {
        USART1_SendChar(*str);
        str++;
    }
}


/*========================================================
 * USART1 SEND NUMBER
 *
 * Chuyển số nguyên thành từng ký tự ASCII
 *========================================================*/
void USART1_SendNumber(unsigned int number)
{
    char buffer[10];
    int i = 0;

    /* Trường hợp số = 0 */
    if (number == 0)
    {
        USART1_SendChar('0');
        return;
    }

    /* Tách từng chữ số từ phải sang trái */
    while (number > 0)
    {
        buffer[i] = (number % 10) + '0';
        i++;

        number = number / 10;
    }

    /* Gửi lại theo thứ tự từ trái sang phải */
    while (i > 0)
    {
        i--;

        USART1_SendChar(buffer[i]);
    }
}


/*========================================================
 * ADC1 INIT
 *
 * ADC1 Channel 3
 * PA3 = Analog Input
 *========================================================*/
void ADC1_Init(void)
{
    /*-----------------------------------------------
     * Bước 1: cấp clock cho GPIOA
     *-----------------------------------------------*/
    RCC_APB2ENR |= (1 << 2);


    /*-----------------------------------------------
     * Bước 2: cấp clock cho ADC1
     *
     * ADC1EN = bit 9
     *-----------------------------------------------*/
    RCC_APB2ENR |= (1 << 9);


    /*-----------------------------------------------
     * Bước 3: cấu hình PA3 làm Analog Input
     *
     * MODE3 = 00
     * CNF3  = 00
     *
     * PA3 nằm trong GPIOA_CRL.
     * Mỗi chân GPIO chiếm 4 bit.
     * PA3 bắt đầu tại bit 12.
     *-----------------------------------------------*/
    GPIOA_CRL &= ~(0xF << 12);


    /*-----------------------------------------------
     * Bước 4: chọn thời gian lấy mẫu cho Channel 3
     *
     * SMP3 = bit 11:9
     *
     * 111 = 239.5 ADC cycles
     *-----------------------------------------------*/
    ADC1_SMPR2 &= ~(7 << 9);
    ADC1_SMPR2 |=  (7 << 9);


    /*-----------------------------------------------
     * Bước 5: chọn số lần chuyển đổi
     *
     * L = 0000
     * → chỉ có 1 conversion
     *-----------------------------------------------*/
    ADC1_SQR1 &= ~(0xF << 20);


    /*-----------------------------------------------
     * Bước 6: chọn Channel 3 cho conversion thứ nhất
     *
     * SQ1 nằm ở SQR3 bit 4:0.
     *-----------------------------------------------*/
    ADC1_SQR3 &= ~(0x1F << 0);
    ADC1_SQR3 |=  (3 << 0);


    /*-----------------------------------------------
     * Bước 7: chọn Software Trigger
     *
     * EXTSEL = bit 19:17
     * 111 = SWSTART
     *-----------------------------------------------*/
    ADC1_CR2 &= ~(7 << 17);
    ADC1_CR2 |=  (7 << 17);


    /*-----------------------------------------------
     * Bước 8: cho phép External Trigger
     *
     * EXTTRIG = bit 20
     *-----------------------------------------------*/
    ADC1_CR2 |= (1 << 20);


    /*-----------------------------------------------
     * Bước 9: bật ADC
     *
     * ADON = bit 0
     *-----------------------------------------------*/
    ADC1_CR2 |= (1 << 0);


    /* Chờ ADC ổn định */
    delay_ms(10);


    /*-----------------------------------------------
     * Bước 10: Reset calibration
     *
     * RSTCAL = bit 3
     *-----------------------------------------------*/
    ADC1_CR2 |= (1 << 3);

    while (ADC1_CR2 & (1 << 3))
    {
    }


    /*-----------------------------------------------
     * Bước 11: bắt đầu calibration
     *
     * CAL = bit 2
     *-----------------------------------------------*/
    ADC1_CR2 |= (1 << 2);

    while (ADC1_CR2 & (1 << 2))
    {
    }
}


/*========================================================
 * ADC1 READ
 *
 * Đọc ADC1 Channel 3 tại PA3.
 *========================================================*/
unsigned int ADC1_Read(void)
{
    /*-----------------------------------------------
     * Xóa cờ EOC
     *
     * EOC = bit 1
     *-----------------------------------------------*/
    ADC1_SR &= ~(1 << 1);


    /*-----------------------------------------------
     * Bắt đầu ADC conversion
     *
     * SWSTART = bit 22
     *-----------------------------------------------*/
    ADC1_CR2 |= (1 << 22);


    /*-----------------------------------------------
     * Chờ chuyển đổi hoàn thành
     *
     * EOC = 1 → conversion hoàn thành
     *-----------------------------------------------*/
    while (!(ADC1_SR & (1 << 1)))
    {
    }


    /*-----------------------------------------------
     * Đọc kết quả ADC
     *
     * ADC 12 bit:
     * 0    → 0V
     * 4095 → 3.3V
     *-----------------------------------------------*/
    return ADC1_DR & 0x0FFF;
}


/*========================================================
 * MAIN
 *========================================================*/
int main(void)
{
    unsigned int adc_value;
    unsigned int voltage_mv;


    /*====================================================
     * KHỞI TẠO USART1
     *====================================================*/
    USART1_Init();

    USART1_SendString("STM32 ADC START\r\n");
    USART1_SendString("CPU/PCLK2 = 8MHz\r\n");
    USART1_SendString("USART1 = 9600 baud\r\n");
    USART1_SendString("ADC = PA3 / Channel 3\r\n");
    USART1_SendString("--------------------\r\n");


    /*====================================================
     * KHỞI TẠO ADC1
     *====================================================*/
    ADC1_Init();

    USART1_SendString("ADC INIT OK\r\n");
    USART1_SendString("--------------------\r\n");


    /*====================================================
     * VÒNG LẶP CHÍNH
     *====================================================*/
    while (1)
    {
        /* Báo bắt đầu đọc ADC */
        USART1_SendString("READ ADC...\r\n");


        /* Đọc giá trị ADC từ PA3 */
        adc_value = ADC1_Read();


        /*-----------------------------------------------
         * In giá trị ADC
         *-----------------------------------------------*/
        USART1_SendString("ADC = ");

        USART1_SendNumber(adc_value);

        USART1_SendString("\r\n");


        /*-----------------------------------------------
         * Đổi ADC sang điện áp
         *
         * Vref = 3.3V
         *
         * Voltage(mV)
         * = ADC × 3300 / 4095
         *-----------------------------------------------*/
        voltage_mv =
            (adc_value * 3300) / 4095;


        /* In điện áp */
        USART1_SendString("Voltage = ");


        /*-----------------------------------------------
         * Phần nguyên của điện áp
         *
         * Ví dụ:
         * 1650 mV → 1.650 V
         *-----------------------------------------------*/
        USART1_SendNumber(voltage_mv / 1000);

        USART1_SendChar('.');


        /*-----------------------------------------------
         * Hàng phần mười
         *-----------------------------------------------*/
        if ((voltage_mv % 1000) < 100)
        {
            USART1_SendChar('0');
        }


        /*-----------------------------------------------
         * Hàng phần trăm
         *-----------------------------------------------*/
        if ((voltage_mv % 1000) < 10)
        {
            USART1_SendChar('0');
        }


        /*-----------------------------------------------
         * Ba chữ số sau dấu chấm
         *-----------------------------------------------*/
        USART1_SendNumber(voltage_mv % 1000);

        USART1_SendString(" V\r\n");


        /* Phân cách giữa các lần đo */
        USART1_SendString("--------------------\r\n");


        /* Đọc lại sau khoảng 1 giây */
        delay_ms(1000);
    }
}
