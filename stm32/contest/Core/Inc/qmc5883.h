#pragma once

#include "main.h"

typedef struct qmc5883l{
    I2C_TypeDef* i2c;
    uint8_t buf[6];
    short x,y,z;
    double axy, axz, ayz;
    short offset_x, offset_y, offset_z;
    short temp;
}qmc_t;

/**
 * 	x = A*x + w; w~N(0,Q)
 * 	y = C*x + v; v~N(0,R)
 */
typedef struct scalar_kalman_s
{
	float A, C;
	float A2, C2; 	// A,C 的平方

	float Q, R;
	float K, P;

	float x;
}scalar_kalman_t;

void scalar_kalman_init(scalar_kalman_t *kalman, float A, float C, float Q, float R);
float scalar_kalman(scalar_kalman_t *kalman, float y);

int qmc_init(I2C_TypeDef *i2c, qmc_t* qmc);
int qmc_get_angle(qmc_t* qmc);
int qmc_check_status(qmc_t* qmc);
int qmc_get_temperature(qmc_t* qmc);
float scalar_kalman(scalar_kalman_t *kalman, float y);
void scalar_kalman_init(scalar_kalman_t *kalman, float A, float C, float Q, float R);

