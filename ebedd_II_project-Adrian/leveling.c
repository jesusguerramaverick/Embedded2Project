

/*
 * levelin.c
 *
 *  Created on: Dec 3, 2021
 *      Author: Arcan
 */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "adc0.h"
#include "uart0.h"
#include "hibernation.h"
#include <time.h>
#define HIBKEY      (*((volatile uint32_t *)(0x400FC030 + (11*4)))) // encryption key
#define HIBSEED     (*((volatile uint32_t *)(0x400FC030 + (12*4)))) // level seed


uint8_t leveling(){
    // static boolean array which will store if the elements of the array are chosen before or not
    static bool chosenElements[10] = {false};
    uint8_t i, j;
    j=0;
    for(i=0; i<10; i++){
        if(chosenElements[i]==true)
            j++;
    }
    if(j==10){
        for(i=0; i<10; i++){
            chosenElements[i]=false;
        }
    }

    // rand() will generate a random integer and if we mod it with 10
    // we will get a number in range 0 to 9
    // this will be used as index of array element
    srand(HIBSEED);//for creating true random using current time

    uint8_t index = rand() % 10;
    // if the number is already chosen before
    while(chosenElements[index] != false){
        // generate a new random number
        index = rand() % 10;
    }

    // mark the index as true in chosenElements array
    chosenElements[index] = true;


    // return the chosen array element
    //return arr[index];
    return index;
}




