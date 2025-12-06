#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Headers/control.h"

// Use system() to capture return value
// /t 2: Wait 2 seconds so the Success message can be sent
int shutdown(){
  return system("c:\\windows\\system32\\shutdown /s /t 2");
}

int restart(){
  return system("c:\\windows\\system32\\shutdown /r /t 2");
}