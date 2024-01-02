# NUC029xEE_SPI_PDMA_32BIT
 NUC029xEE_SPI_PDMA_32BIT

update @ 2024/01/02

1. initial SPI TX with PDMA interrupt enable

2. test SPI 32 bit data width , 16 BYTES , with different MODE

- MODE0 : CPOL:LOW,CPHA:LOW 

- MODE1 : CPOL:LOW,CPHA:HIGH 

- MODE2 : CPOL:HIGH,CPHA:LOW 

- MODE3 : CPOL:HIGH,CPHA:HIGH 

3. press digit 1 , will send SPI TX (16 BYTES) with PDMA ,

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_MODE0_1.jpg)	

3-1 , below is MODE0 capture

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_MODE0_2.jpg)	

3-2 , below is MODE1 capture

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_MODE1_2.jpg)	

3-3 , below is MODE2 capture

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_MODE0_2.jpg)	

3-4 , below is MODE3 capture

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_MODE3_2.jpg)	


4. press digit 2 , will send SPI TX normal flow

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/LA_SPI_nonPDMA.jpg)	

5. below is log capture

![image](https://github.com/released/NUC029xEE_SPI_PDMA_32BIT/blob/main/log.jpg)	

