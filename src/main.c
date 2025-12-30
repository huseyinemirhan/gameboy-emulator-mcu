	/*
 * main.c
 *
 *  Created on: Dec 30, 2025
 *      Author: MSI Cyborg
 */

#include "cpu/cpu.h"
#include "memory/memory.h"
#include <stdio.h>

void main(){

	CPU_Init();

	for(int i = 0; i<10; i++){
		CPU_Step();
		printf("%d\n",cpu.cycles);
		printf("%d\n",cpu.A);
	}

}


