/*
 * enums.h
 *
 *  Created on: Sep 10, 2018
 *      Author: fernando
 */

#ifndef ENUMS_H_
#define ENUMS_H_

enum conexiones
{
	NUEVA_CONEXION_DMA = 1,
	NUEVA_CONEXION_CPU = 2,
	NUEVA_CONEXION_MDJ = 3,
	NUEVA_CONEXION_FM9 = 4,
	NUEVA_CONEXION_SAFA = 5,
	MSJ_DESDE_DMA = 6,
	MSJ_DESDE_CPU = 7,
	MSJ_DESDE_MDJ = 8,
	MSJ_DESDE_FM9 = 9,
	MSJ_DESDE_SAFA = 10
};

enum exit_codes
{
	ExitCode_CONFIGNOTFOUND = -10
};

enum tareas
{

};

#endif /* ENUMS_H_ */
