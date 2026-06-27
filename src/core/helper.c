#include "helper.h"

// general push and pop function
void PushCharStack(char *array, char value)
{
    int size = strlen(array);
    printf("size: %d - char: %c\n", size, value);
   
    array[size] = value;
    array[size + 1] = '\0';
}

void PopStack(char *array)
{
    int size = strlen(array);
    if (size == 0)
    {
        printf("Cannot pop an empty string.\n");
        return;
    }

    char popped = array[size - 1];
    printf("popped: %c\n", popped);

    array[size - 1] = '\0';
}

// small helper function
void PushString(char *dest, char *src)
{
    int index = 0;
    int size = strlen(src);
    for (int i = 0; i < size; ++i)
    {
        dest[index] = src[i];
        dest[index + 1] = '\0';
        ++index;
    }
}


