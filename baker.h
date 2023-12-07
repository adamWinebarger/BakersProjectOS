#ifndef baker_h
#define baker_h

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>

//Recipe felt like it also needed a data structure to keep track of everything
typedef struct {
    int id;
    char *name;
    bool ingredientsNeeded[9];
    bool ingredientsAccquired[9];
    bool allIngredientsAccquired;
    bool mixed;
    bool ovened;
    bool complete;

} Recipe;

//Data structure for our bakers
typedef struct {
    int id;
    char *name;
    sem_t currentResource; //seems like we should have a way top track what resource is currently in use by a baker in addition to what recipe they're doing
    //currentResource actually might end up being totally useless
    bool hasBowl, hasSpoon, hasMixer, hasOven;
    Recipe currentRecipe; //I think it would  be better if we had this pass a copy of each Recipe rather than a pointer to the recipe itself.
    bool recipesCooked[5];
    char* textColor;
} Baker;

//Order of ingredients
//Flour, Sugar, Yeast, BakingSoda, Salt, Cinnamon, Eggs, Milk, Butter

Recipe Cookies = {
    0,
    "Cookies",
    {true, true, false, false, false, false, false, true, true},
    {false, false, false, false, false, false, false, false, false},
    false,
    false,
    false,
    false
};

//Order of ingredients
//Flour, Sugar, Yeast, BakingSoda, Salt, Cinnamon, Eggs, Milk, Butter
Recipe Pancakes = {
    1,
    "Pancakes",
    {true, true, false, true, true, false, true, true, true},
    {false, false, false, false, false, false, false, false, false},
    false,
    false,
    false,
    false
};

//Order of ingredients
//Flour, Sugar, Yeast, BakingSoda, Salt, Cinnamon, Eggs, Milk, Butter
Recipe PizzaDough = {
    2,
    "Homemade_Pizza_Dough",
    {false, true, true, false, true, false, false, false, false},
    {false, false, false, false, false, false, false, false, false},
    false,
    false,
    false,
    false
};

//Order of ingredients
//Flour, Sugar, Yeast, BakingSoda, Salt, Cinnamon, Eggs, Milk, Butter
Recipe SoftPretzel = {
    3,
    "Soft_Pretzel",
    {true, true, true, true, true, false, true, false, false},
    {false, false, false, false, false, false, false, false, false},
    false,
    false,
    false,
    false
};

Recipe CinnamonRoll  = {
    4,
    "Cinnamon_Roll",
    {true, true, false, true, true, true, true, false, true},
    {false, false, false, false, false, false, false, false, false},
    false,
    false,
    false,
    false
};

#endif
