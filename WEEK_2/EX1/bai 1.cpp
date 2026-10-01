/*
 * ============================================================
 *              USART1 - STM32F103C8T6
 * ============================================================
 *
 * Chức năng:
 * - Giao tiếp UART giữa STM32 và máy tính.
 * - PA9  = TX, PA10 = RX.
 * - Baud Rate = 115200, cấu hình 8N1.
 * - Nhận dữ liệu, lưu vào buffer.
 * - Ký tự '!' dùng để báo kết thúc bản tin.
 *
 * Ví dụ:
 * PC gửi: HELLO!
 * STM32 trả: D23HTNN01: HELLO
 *
 * Chương trình sử dụng Polling, liên tục kiểm tra TXE/RXNE.
 */


/* RCC_APB2ENR: bật clock cho các ngoại vi APB2.
 *
 * Bit 2  = IOPAEN   → bật clock GPIOA.
 * Bit 14 = USART1EN → bật clock USART1.
 */
#define RCC_APB2ENR (*(volatile unsigned int *)0x40021018)


/* GPIOA_CRH dùng cấu hình PA8 → PA15.
 *
 * PA9  = USART1_TX
 * PA10 = USART1_RX
 */
#define GPIOA_CRH   (*(volatile unsigned int *)0x40010804)


/* USART1_SR: thanh ghi trạng thái.
 *
 * Bit 7 = TXE  → thanh ghi truyền rỗng.
 * Bit 5 = RXNE → đã nhận được dữ liệu.
 */
#define USART1_SR   (*(volatile unsigned int *)0x40013800)


/* USART1_DR: thanh ghi dữ liệu.
 *
 * Ghi DR → truyền dữ liệu.
 * Đọc DR → nhận dữ liệu.
 */
#define USART1_DR   (*(volatile unsigned int *)0x40013804)


/* USART1_BRR: thanh ghi cài đặt Baud Rate.
 *
 * PCLK2 = 8 MHz
 * Baud  = 115200
 *
 * Công thức:
 * USARTDIV = PCLK2 / (16 × Baudrate)
 *
 * Với bài này: BRR = 0x0045.
 */
#define USART1_BRR  (*(volatile unsigned int *)0x40013808)


/* USART1_CR1: thanh ghi điều khiển.
 *
 * Bit 13 = UE → bật USART.
 * Bit 3  = TE → bật truyền.
 * Bit 2  = RE → bật nhận.
 */
#define USART1_CR1  (*(volatile unsigned int *)0x4001380C)


/*
 * Buffer nhận dữ liệu có 64 byte.
 *
 * Tối đa 63 ký tự dữ liệu,
 * 1 byte còn lại dành cho '\0'.
 */
#define BUFFER_SIZE 64

char rx_buffer[BUFFER_SIZE];


/*
 * rx_index lưu vị trí tiếp theo cần ghi vào buffer.
 *
 * Ban đầu = 0.
 * Sau mỗi ký tự nhận được thì tăng lên 1.
 */
unsigned int rx_index = 0;


/*
 * ============================================================
 *                    KHỞI TẠO USART1
 * ============================================================
 *
 * Các bước:
 * 1. Bật clock GPIOA.
 * 2. Bật clock USART1.
 * 3. Cấu hình PA9 = TX.
 * 4. Cấu hình PA10 = RX.
 * 5. Cài Baud Rate = 115200.
 * 6. Bật USART, TX và RX.
 */
void USART1_Init(void)
{
    /* Bật clock GPIOA - bit 2 */
    RCC_APB2ENR |= (1 << 2);

    /* Bật clock USART1 - bit 14 */
    RCC_APB2ENR |= (1 << 14);

    /*
     * PA9  = TX → Alternate Function Push-Pull 50MHz.
     * PA10 = RX → Input Floating.
     */
    GPIOA_CRH &= 0xFFFFF00F;
    GPIOA_CRH |= 0x000004B0;

    /*
     * PCLK2 = 8 MHz, Baud = 115200.
     * BRR = 0x0045.
     */
    USART1_BRR = 0x0045;

    /*
     * UE  = bit 13 → bật USART.
     * TE  = bit 3  → bật truyền.
     * RE  = bit 2  → bật nhận.
     */
    USART1_CR1 = (1 << 13)
               | (1 << 3)
               | (1 << 2);
}


/*
 * ============================================================
 *                    GỬI 1 KÝ TỰ
 * ============================================================
 *
 * Chờ TXE = 1 rồi ghi ký tự vào DR.
 * TXE nằm ở bit 7 của USART1_SR.
 */
void USART1_SendChar(char c)
{
    /* Chờ thanh ghi truyền rỗng */
    while (!(USART1_SR & (1 << 7)))
    {
    }

    /* Ghi dữ liệu vào DR để truyền */
    USART1_DR = c;
}


/*
 * ============================================================
 *                    GỬI CHUỖI
 * ============================================================
 *
 * Gửi lần lượt từng ký tự trong chuỗi.
 * Chuỗi C kết thúc bằng '\0'.
 */
void USART1_SendString(const char *str)
{
    while (*str)
    {
        USART1_SendChar(*str);
        str++;
    }
}


/*
 * ============================================================
 *                    NHẬN 1 KÝ TỰ
 * ============================================================
 *
 * Chờ RXNE = 1 rồi đọc dữ liệu từ DR.
 * RXNE nằm ở bit 5 của USART1_SR.
 */
char USART1_ReceiveChar(void)
{
    /* Chờ cho tới khi có dữ liệu nhận */
    while (!(USART1_SR & (1 << 5)))
    {
    }

    /* Đọc 8 bit dữ liệu từ DR */
    return (char)(USART1_DR & 0xFF);
}


/*
 * ============================================================
 *                         MAIN
 * ============================================================
 */
int main(void)
{
    /* Khởi tạo USART1 */
    USART1_Init();

    /* Thông báo USART đã khởi động */
    USART1_SendString("STM32 UART START\r\n");

    while (1)
    {
        char c;

        /* Nhận một ký tự từ máy tính */
        c = USART1_ReceiveChar();

        /*
         * Nếu nhận '!' thì bản tin kết thúc.
         */
        if (c == '!')
        {
            /*
             * Thêm '\0' để biến dữ liệu trong buffer
             * thành chuỗi C.
             */
            rx_buffer[rx_index] = '\0';

            /* Gửi mã lớp/nhóm */
            USART1_SendString("D23HTNN01: ");

            /* Gửi lại nội dung đã nhận */
            USART1_SendString(rx_buffer);

            /* Xuống dòng */
            USART1_SendString("\r\n");

            /*
             * Xử lý xong bản tin,
             * đưa index về 0 để nhận bản tin mới.
             */
            rx_index = 0;
        }
        else
        {
            /*
             * Nếu chưa đầy buffer:
             * lưu ký tự vào vị trí hiện tại.
             */
            if (rx_index < BUFFER_SIZE - 1)
            {
                rx_buffer[rx_index] = c;
                rx_index++;
            }
            else
            {
                /*
                 * Buffer đầy → reset index
                 * để bắt đầu ghi lại từ đầu.
                 */
                rx_index = 0;
            }
        }
    }
}
