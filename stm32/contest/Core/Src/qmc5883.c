#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "qmc5883.h"
#include <math.h>


#define QMC5883_ADDR        0x0d
#define QMC5883_WRITE       0
#define QMC5883_READ        1

enum{
    XOUT_LOW = 0x00,
    XOUT_HIGH,
    YOUT_LOW,
    YOUT_HIGH,
    ZOUT_LOW,
    ZOUT_HIGH,
    STATUS,
    TEMP_LOW,
    TEMP_HIGH,
    CONTROL1,
    CONTROL2,
    SET_RESET_PERIOD,
    RESERVERD,
    CHIP_ID,
};

enum{
    MODE_STANDBY    = 0x0,
    MODE_CONTINUOUS = 0x1,
    DATA_RATE_10    = (0x0<<2),
    DATA_RATE_50    = (0x1<<2),
    DATA_RATE_100   = (0x2<<2),
    DATA_RATE_200   = (0x3<<2),
    FULL_SCALE_2G   = (0x0<<4),
    FULL_SCALE_8G   = (0x1<<4),
    SAMPLE_RATIO_512= (0x0<<6),
    SAMPLE_RATIO_256= (0x1<<6),
    SAMPLE_RATIO_128= (0x2<<6),
    SAMPLE_RATIO_64 = (0x3<<6),
};

enum{
    SOFT_RST        = 0x80,
    ROL_PNT         = 0x40,
    INT_ENB         = 0x01,
};

enum{
    STATUS_DRDY     = 0x1,
    STATUS_OVL      = 0x2,
    STATUS_DOR      = 0x4, 
};

int qmc_write_byte(I2C_TypeDef *i2c, uint8_t reg, uint8_t data){
    int cnt = 0;
    while(LL_I2C_IsActiveFlag_BUSY(i2c)){
        ++cnt;
        if(cnt > 25000){
            printf("qmc5883l i2c is busy\r\n");
            return 0;
        }
    }
    LL_I2C_GenerateStartCondition(i2c);
    while(!LL_I2C_IsActiveFlag_SB(i2c));

    LL_I2C_TransmitData8(i2c, (QMC5883_ADDR<<1)|QMC5883_WRITE);
    while(!LL_I2C_IsActiveFlag_ADDR(i2c));
    LL_I2C_ClearFlag_ADDR(i2c);

    LL_I2C_TransmitData8(i2c, reg);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_TransmitData8(i2c, data);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_GenerateStopCondition(i2c);

    return data;
}

int qmc_read_byte(I2C_TypeDef *i2c, uint8_t reg, uint8_t* data){
    int cnt = 0;
    while(LL_I2C_IsActiveFlag_BUSY(i2c)){
        ++cnt;
        if(cnt > 25000){
            printf("qmc5883l i2c is busy\r\n");
            return 0;
        }
    }
    LL_I2C_GenerateStartCondition(i2c);
    while(!LL_I2C_IsActiveFlag_SB(i2c));

    LL_I2C_TransmitData8(i2c, (QMC5883_ADDR<<1)|QMC5883_WRITE);
    while(!LL_I2C_IsActiveFlag_ADDR(i2c));
    LL_I2C_ClearFlag_ADDR(i2c);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_TransmitData8(i2c, reg);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_GenerateStartCondition(i2c);
    while(!LL_I2C_IsActiveFlag_SB(i2c));

    LL_I2C_TransmitData8(i2c, (QMC5883_ADDR<<1)|QMC5883_READ);
    while(!LL_I2C_IsActiveFlag_ADDR(i2c));
    LL_I2C_ClearFlag_ADDR(i2c);

    while(!LL_I2C_IsActiveFlag_RXNE(i2c));
    *data = LL_I2C_ReceiveData8(i2c);

    LL_I2C_GenerateStopCondition(i2c);
    

    return *data;
}

int qmc_read_xyz(I2C_TypeDef *i2c, uint8_t begin_reg, uint8_t* data){
    int cnt = 0;
    while(LL_I2C_IsActiveFlag_BUSY(i2c)){
        ++cnt;
        if(cnt > 25000){
            printf("qmc5883l i2c is busy\r\n");
            return 0;
        }
    }
    LL_I2C_GenerateStartCondition(i2c);
    while(!LL_I2C_IsActiveFlag_SB(i2c));

    LL_I2C_TransmitData8(i2c, (QMC5883_ADDR<<1)|QMC5883_WRITE);
    while(!LL_I2C_IsActiveFlag_ADDR(i2c));
    LL_I2C_ClearFlag_ADDR(i2c);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_TransmitData8(i2c, begin_reg);
    while(!LL_I2C_IsActiveFlag_TXE(i2c));

    LL_I2C_GenerateStartCondition(i2c);
    while(!LL_I2C_IsActiveFlag_SB(i2c));

    LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
    LL_I2C_TransmitData8(i2c, (QMC5883_ADDR<<1)|QMC5883_READ);
    while(!LL_I2C_IsActiveFlag_ADDR(i2c));
    LL_I2C_ClearFlag_ADDR(i2c);
    for(int i = 0; i < 5; ++i){
        while(!LL_I2C_IsActiveFlag_RXNE(i2c));
        data[i] = LL_I2C_ReceiveData8(i2c);
    }
    LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
    while(!LL_I2C_IsActiveFlag_RXNE(i2c));
    data[5] = LL_I2C_ReceiveData8(i2c);
    
    LL_I2C_GenerateStopCondition(i2c);
    return 1;
}

int qmc_check_i2c(I2C_TypeDef *i2c){
    uint8_t check_code = 0;
    qmc_read_byte(i2c, CHIP_ID, &check_code);
    return check_code==0xff;
}

int qmc_check_status(qmc_t* qmc){
    uint8_t tmp;
    qmc_read_byte(qmc->i2c, STATUS, &tmp);
    return tmp;
}

int qmc_init(I2C_TypeDef *i2c, qmc_t* qmc){
    memset(qmc, 0, sizeof(qmc_t));
    qmc->i2c = i2c;
    LL_I2C_AcknowledgeNextData(qmc->i2c, LL_I2C_NACK);
    if(!LL_I2C_IsEnabled(qmc->i2c)){
        LL_I2C_Enable(qmc->i2c);
    }
    if(qmc_check_i2c(qmc->i2c) != 1){
        printf("qmc5883l i2c init failed\r\n");
        return -1;
    }
    uint8_t tmp;
    qmc_write_byte(qmc->i2c, CONTROL2, SOFT_RST);
    osDelay(100/portTICK_PERIOD_MS);

    qmc_write_byte(qmc->i2c, SET_RESET_PERIOD, 0x01);

    uint8_t config = 0;
    config = (SAMPLE_RATIO_512)|(FULL_SCALE_2G)|(DATA_RATE_100)|(MODE_CONTINUOUS);
    qmc_write_byte(qmc->i2c, CONTROL1, config);
    if(qmc_read_byte(qmc->i2c, CONTROL1, &tmp) != config){
        printf("write qmc5883l control1 register failed\r\n");
    }

    config = ROL_PNT;
    qmc_write_byte(qmc->i2c, CONTROL2, config);
    if(qmc_read_byte(qmc->i2c, CONTROL2, &tmp) != config){
        printf("write qmc5883l control2 register failed\r\n");
    }
    
    printf("init qmc5883l:ok\r\n");
    return 0;
}

double mag2yaw(uint16_t x, uint16_t y){
    double angle = atan2((double)y, (double)x)*(180 / 3.14159265)+180;
    return angle;
}

int qmc_get_angle(qmc_t* qmc){
    uint8_t data = qmc_check_status(qmc);
    if((data&STATUS_DRDY) == 0){
        printf("qmc5883 data is not ready\r\n");
        return 0;
    }
    qmc_read_xyz(qmc->i2c, XOUT_LOW, qmc->buf);
    short tmp = ((qmc->buf[1]<<8)|qmc->buf[0]);
    qmc->x = tmp/2+qmc->x/2;
    tmp = ((qmc->buf[3]<<8)|qmc->buf[2]);
    qmc->y = qmc->y/2+tmp/2;
    tmp = ((qmc->buf[5]<<8)|qmc->buf[4]);
    qmc->z = tmp/2+qmc->z/2;
    qmc->axy = mag2yaw(qmc->x, qmc->y);
    qmc->axz = mag2yaw(qmc->x, qmc->z);
    qmc->ayz = mag2yaw(qmc->y, qmc->z);
    return 1;
}

int qmc_calibration(qmc_t* qmc){
    short y_max = 0, y_min = 0, z_max = 0, z_min = 0;
    while(1){
        qmc_read_xyz(qmc->i2c, XOUT_LOW, qmc->buf);
        y_max = (qmc->y>y_max)?qmc->y:y_max;
        y_min = (qmc->y<y_min)?qmc->y:y_min;
        z_max = (qmc->z>z_max)?qmc->z:z_max;
        z_min = (qmc->z<z_min)?qmc->z:z_min;
        
    }
}

int qmc_get_temperature(qmc_t* qmc){
    uint8_t l = 0,h = 0;
    qmc_read_byte(qmc->i2c, TEMP_LOW, &l);
    qmc_read_byte(qmc->i2c, TEMP_HIGH, &h);
    qmc->temp = (h<<8)|l;
    return qmc->temp;
}

/**
* 	状态方程: x = A*x + w; w~N(0,Q)
* 	测量方程: y = C*x + v; v~N(0,R)
 */

float scalar_kalman(scalar_kalman_t *kalman, float y)
{
	// 状态预测
	kalman->x = kalman->A * kalman->x;
	// 误差协方差预测
	kalman->P = kalman->A2 * kalman->P + kalman->Q;
	// 计算卡尔曼滤波增益
	kalman->K = kalman->P * kalman->C / (kalman->C2 * kalman->P + kalman->R);
	
	// 状态估计校正
	kalman->x = kalman->x + kalman->K * (y - kalman->C * kalman->x);
	// 误差协方差估计校正
	kalman->P = (1 - kalman->K * kalman->C) * kalman->P;
	
	return kalman->C * kalman->x; 	// 输出滤波后的y
}

void scalar_kalman_init(scalar_kalman_t *kalman, float A, float C, float Q, float R)
{
	kalman->A = A;
	kalman->A2 = A * A;
	kalman->C = C;
	kalman->C2 = C * C;

	kalman->Q = Q;
	kalman->R = R;

	kalman->x = 0;
	kalman->P = Q;
	kalman->K = 1;
}

