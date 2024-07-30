#include <wiringPi.h>
#include <stdio.h>

int main(void) {
    if (wiringPiSetup() == -1) {
        printf("wiringPi setup failed.\n");
        return 1;
    }

    printf("wiringPi setup successful.\n");
    return 0;

      pinMode(26, INPUT);
      pinMode(25, INPUT);
      pinMode(24, INPUT);

}
