#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>

#include "baker.h"
#include "color.h"

int numBakers;//Might want to leave this open so then we don't have to worry about trying to pass it to our sigHandler



//Need to define semaphores for each of our available resources
sem_t mixerSem, pantrySem, refridgeratorSem, bowlSem, spoonSem, ovenSem;

//Semaphores to handle ingredients in the pantry and refridgerator
sem_t pantryIngredients[6],
    fridgeIngredients[3];

//Let's do prototyping for all our other functions in this file though
void* bakerThread(void* arg);
void cleanup();
bool checkAllRecipesCooke(bool* recipes, int length);
void  accquireResource(sem_t *resource_sem, char *resource_name, char *bakerName, int id, char* color);
void releaseResource(sem_t *resource_sem, char *resource_name, char *bakerName, int id, char* color);
bool getIngredients(Baker *baker);
bool checkAllIngredientsRequired(Baker *baker, int start, int end);
bool getRamsied();
void gotRamsied(Baker *baker);
void getSpoonsNShit(Baker *baker);
bool semCheck(sem_t* sema, char* name, Baker* baker);

void sigHandler() {
    //Going to need to handle cleanup here for if anyone is to give SIGINT, SIGKILL, or something like a segFault
    cleanup();
    exit(0);
}

int main(int argc, char *argv[])
{
    time_t t;
    srand((unsigned) time(&t)); //kind of want to add a bit of randomness for what recipes the guys decide to select first
    numBakers = (argc >= 2 && atoi(argv[1]) != 0) ? atoi(argv[0]) : 3;

    char* Colors[] = {RED, GREEN, YELLOW, BLUE, PURPLE};
    //first thing's first, let's set up our sigHandler
    signal(SIGINT, sigHandler);
    signal(SIGKILL, sigHandler);

    //printf("\033[1;31m");
    //printf(“Hello\n”);
    //printf("%s\n", Cookies.name);

    int i; //this will be less computationally expensive, I guess
    pthread_t bakerThreads[numBakers];
    Baker bakerData[numBakers];

    //Now let's initialize the semaphores and then come back to mess with the bakers
    sem_init(&mixerSem, 0, 2); //In theory, we should be able to decrement or incremnent our semValue to indicate what's in use.
    sem_init(&pantrySem, 0, 1); //If that doesn't work, then we'll have to turn our mixers and things into semArrays and initialize
    sem_init(&refridgeratorSem, 0, 2); //each of them individually... should be good with what we got though
    sem_init(&bowlSem, 0, 3);
    sem_init(&spoonSem, 0, 5);
    sem_init(&ovenSem, 0, 1);

    //now to do our ingredients
    for (i = 0; i < 6; i++)
        sem_init(&pantryIngredients[i], 0, 1);

    for (i = 0; i < 3; i++)
        sem_init(&fridgeIngredients[i], 0, 2);

    //Creating our bakerThreads
    for (i = 0; i < numBakers; i++){
        bakerData[i].id = i;
        //bakerData[i].name = malloc(sizeof(char) * 10);
        bakerData[i].name = "Baker";
        bakerData[i].textColor = Colors[i % 5];
        pthread_create(&bakerThreads[i], NULL, bakerThread, &bakerData[i]);
    }

    //Gotta join our bakerThreads
    for (i = 0; i < numBakers; i++) {
        pthread_join(bakerThreads[i], NULL);
        //pthread_exit(NULL);
    }
    //pthread_exit(NULL);
    //Alright. So if we want realtimeness, then we need to do our initialization and assignment of recipes up here
    //and then push down the individual tasks to the baker threads.

    //So let's initializ our bakers first
    // for (i = 0; i < numBakers; i++) {
    //     bakerData[i].id = i;
    //     bakerData[i].name = "Baker";
    //     bakerData[i].textColor = Colors[i];
    //
    // }

    //Cleanup Process here
    cleanup();

    //printf("%x\n", true);
    return 0;
}

void* bakerThread(void* arg)
{
    int i;
    Baker *baker = (Baker*) arg;
    Recipe recipes[] = {Cookies, Pancakes, PizzaDough, SoftPretzel, CinnamonRoll};

    //We're getting kind of weird with our initialization but I guess may as well do some of it down here too
    baker->hasBowl  = false;
    baker->hasMixer = false;
    baker->hasSpoon = false;
    baker->hasOven = false;

    for (i = 0; i < 5; i++)
        baker->recipesCooked[i] = false; //initalizing cooked-record to false accross the board

    bool allRecipesCooked = false;

    //This seems like a decent enough way to implement the logic here
    while (!allRecipesCooked) {

        int selectedRecipe = rand() % 5;

        while (baker->recipesCooked[selectedRecipe]) {
            printf("%s%s %d: Already cooked recipe %s. Selecting another...\n", baker->textColor, baker->name, baker->id+1,
                   recipes[selectedRecipe].name);
            selectedRecipe = rand() % 5;    //this way not every baker should be working on the same recipe at the same time
        }

        baker->currentRecipe = recipes[selectedRecipe];
        //printf("%s%d\n",  baker->textColor, selectedRecipe);
        printf("%s%s %d Started on new recipe: %s\n", baker->textColor, baker->name, (baker->id + 1), baker->currentRecipe.name);
        //This should be where we start building our recipe.
        while (baker->currentRecipe.complete == false) {
            //printf("Banana\n");

            int semVal; //this can be used to check and see what the value is in our different semaphores

            while (baker->currentRecipe.allIngredientsAccquired == false) {

                //printf("%sIn while loop\n", baker->textColor);
                baker->currentRecipe.allIngredientsAccquired = getIngredients(baker);
                //sleep(1);
            }
            //printf("%s%s %d: All ingredients accquired\n", baker->textColor, baker->name, baker->id+1);
            if (getRamsied())   //fuck it, we're going to run the ramsey chance after every stage of the process
                gotRamsied(baker);

            //Now we gotta move on to getting the spoons n shit
            while (!baker->hasBowl && !baker->hasSpoon && !baker->hasMixer)
                getSpoonsNShit(baker);
            //sleep(1);

            if (getRamsied())
                gotRamsied(baker);

            //Ok. Now we need to wait for the oven
            while (!baker->hasOven) {
                baker->hasOven = semCheck(&ovenSem, "Oven", baker);
                if (baker->hasOven) {
                    printf("%s%s %d: Oven accquired. Now cooking\n",  baker->textColor, baker->name, baker->id+1);
                    printf("%s%s %d: Cooking completed\n", baker->textColor, baker->name, baker->id+1);
                    releaseResource(&ovenSem, "Oven", baker->name,  baker->id, baker->textColor);
                    baker->currentRecipe.complete = true;
                    //sleep(1);
                }
            }


            if (getRamsied())
                gotRamsied(baker);

            if  (baker->currentRecipe.complete) {
                printf("%s%s %d: Recipe %s complete\n",  baker->textColor, baker->name,  baker->id+1, baker->currentRecipe.name);
                baker->recipesCooked[baker->currentRecipe.id] = true;
                baker->hasBowl =  false;
                baker->hasMixer = false;
                baker->hasSpoon = false;
                baker->hasOven = false;
                //sleep(1);
            }

        }

        //check at the end here if all recipes have been cooked
        allRecipesCooked = checkAllRecipesCooke(baker->recipesCooked, 5);
        //pthread_exit(NULL);
    }
    printf("%s%s %d: All recipes Complete!\n", baker->textColor, baker->name, baker->id+1);

    pthread_exit(NULL);

}

//Actually, I think it would make more sense to increment ID here rather than doing so for every functioncall
void  accquireResource(sem_t *resource_sem, char *resource_name, char *bakerName, int id, char* color) {
    printf("%s%s %d: Accquiring %s\n", color, bakerName, id+1, resource_name);
    sem_wait(resource_sem);
}

void releaseResource(sem_t *resource_sem, char *resource_name, char*  bakerName, int id, char* color) {
    printf("%s%s %d: Releasing %s\n", color, bakerName, id+1, resource_name);
    sem_post(resource_sem);
}

void getSpoonsNShit(Baker *baker) {

    int semVal;

    //I guess let's handle the spoon first since we mentioned it
    if (!baker->hasSpoon)
        baker->hasSpoon = semCheck(&spoonSem, "Spoon",  baker);

    if (getRamsied())   //fuck it, we're going to run the ramsey chance after every stage of the process
            gotRamsied(baker);

    if (!baker->hasBowl) {
        baker->hasBowl = semCheck(&bowlSem, "Bowl", baker);
    }

    if (getRamsied())
            gotRamsied(baker);

    if (!baker->hasMixer) {
        baker->hasMixer = semCheck(&mixerSem, "Mixer", baker);
    }
    if (getRamsied())
            gotRamsied(baker);

    if (baker->hasBowl && baker-> hasSpoon && baker->hasMixer) {
        printf("%s%s %d: All mixins required. Preparing to mix\n", baker->textColor, baker->name, baker->id +1);
        printf("%s%s %d: Mixing complete. Returning the tools to their appropriate places\n", baker->textColor, baker->name, baker->id +1);
        releaseResource(&bowlSem, "Bowl", baker->name, baker->id, baker->textColor);
        releaseResource(&spoonSem, "Spoon", baker->name, baker->id, baker->textColor);
        releaseResource(&mixerSem, "Mixer", baker->name, baker->id, baker->textColor);
    }
}

bool semCheck(sem_t* sema, char* name, Baker* baker)
{
    int semVal;
    sem_getvalue(sema, &semVal);
    printf("%s%s %d: Searching for %s\n",  baker->textColor, baker->name, baker->id+1, name);
    if  (semVal == 0)
        printf("%s%s %d: All %ss currently in use. Moving on\n", baker->textColor, baker->name, baker->id+1, name);
    else {
        accquireResource(sema, name, baker->name, baker->id, baker->textColor);
        return true;
    }
    return false;
}


bool getIngredients(Baker *baker) {
    int semVal, i, realID = baker->id + 1;
    char* ingredientNames[] = {"Flour", "Sugar", "Yeast", "Baking_Soda", "Salt", "Cinnaomon", "Eggs", "Milk", "Butter"};

    if (!checkAllIngredientsRequired(baker, 6, 9)) { //yet another catchment to make sure that bakers aren't going to the fridge/pantry if they don't need to
        //printf("%s%s %d accquiring ingredients from fridge.\n", baker->textColor, baker->name,  realID);
        sem_getvalue(&refridgeratorSem, &semVal);//checking availability of the fridge

        if (semVal != 0) {
            accquireResource(&refridgeratorSem, "Refrigerator", baker->name, baker->id, baker->textColor);
            //Case where the fridge is available
            //int ingredientVal = -1;

            //Gotta remember 6, 7, 8 because those are the fridge items in our arrays... since we wanted to have separate arrays for fridge vs pantry
            for (i = 0; i < 3; i++) {
                if (baker->currentRecipe.ingredientsAccquired[i+6] != baker->currentRecipe.ingredientsNeeded[i+6])  {
                    //ingredientVal = i; //should  have done the pantry first... oh well.
                    printf("%s%s %d: Checking for %s in fridge\n", baker->textColor, baker->name, realID, ingredientNames[i+6]);
                    sem_getvalue(&fridgeIngredients[i], &semVal);
                    if (semVal != 0) {
                        //case we found the ingredient that we were looking for
                        accquireResource(&fridgeIngredients[i], ingredientNames[i+6], baker->name, baker->id, baker->textColor);
                        baker->currentRecipe.ingredientsAccquired[i+6] = true; //god we should really go back and optimize this

                        //releaseResource(&fridgeIngredients[i], ingredientNames[i+6], baker->name, baker->id); //Quick grab and put it back I guess
                        //I think we can avoid deadlock if we give fridge ingredients x2 (for the 2 fridges... seems like
                        // we'll really be depending on the ramsey factor to keep this moving along though
                        //Worst, case, rememer to uncomment the release statement in here.
                        //Maybe we should consider other ways to release the ingredients after they've been accessed though... tough call
                        //Ideas: Release all ingredients when all have been accquired, getRamsied, put ingredients back in the event of deadlock
                        //break; //I only want them to get 1 ingredient at a time
                        //Removing the break statement might be the best course of action here since that would mean that a chef is getting everything they
                        //need at the same time
                        releaseResource(&fridgeIngredients[i], ingredientNames[i+6], baker->name, baker->id, baker->textColor);
                    } else {
                        printf("%s%s %d: %s in use, moving on\n", baker->textColor, baker->name, realID, ingredientNames[i+6]);
                    }
                }
            }
            releaseResource(&refridgeratorSem, "Refrigerator", baker->name, baker->id, baker->textColor);
        } else
            printf("%s%s %d: Fridge in use, moving on\n", baker->textColor, baker->name,  realID);
    }

    if (!checkAllIngredientsRequired(baker, 0, 6)) {
        //Now we move on to checking the pantry
        printf("%s%s %d: Checking pantry: \n", baker->textColor, baker->name, realID);
        sem_getvalue(&pantrySem, &semVal);

        if (semVal == 0) {
            printf("%s%s %d: Pantry in use, moving on.\n", baker->textColor, baker->name, realID);
        } else {
            accquireResource(&pantrySem, "Pantry", baker->name, baker->id, baker->textColor);

            for (i = 0; i < 6; i++)
                if (baker->currentRecipe.ingredientsAccquired[i] != baker->currentRecipe.ingredientsNeeded[i]) {
                    printf("%s%s %d: Checking Pantry for %s\n", baker->textColor, baker->name, realID, ingredientNames[i]);
                    sem_getvalue(&pantryIngredients[i], &semVal);
                    if (semVal == 0) {
                        printf("%s%s %d: %s in use. Moving on.\n", baker->textColor, baker->name, realID, ingredientNames[i]);
                    } else {
                        accquireResource(&pantryIngredients[i], ingredientNames[i], baker->name, baker->id, baker->textColor);
                        baker->currentRecipe.ingredientsAccquired[i] = true;
                        releaseResource(&pantryIngredients[i], ingredientNames[i], baker->name, baker->id,  baker->textColor);

                    }
                }
            releaseResource(&pantrySem, "Pantry", baker->name, baker->id, baker->textColor);
        }
    }

    for (i = 0; i < 9; i++)
        if (baker->currentRecipe.ingredientsAccquired[i] != baker->currentRecipe.ingredientsNeeded[i])
            return false;

    //Now we gotta make sure to do a semRelease on all of the ingredients we just got from our thing
    // for (i = 0; i < 6; i++)
    //     if (baker->currentRecipe.ingredientsAccquired[i])
    //         releaseResource(&pantryIngredients[i], ingredientNames[i], baker->name, baker->id, baker->textColor);
    // for (i = 0; i < 3; i++)
    //     if (baker->currentRecipe.ingredientsAccquired[i+6])
    //         releaseResource(&fridgeIngredients[i], ingredientNames[i+6], baker->name, baker->id,  baker->textColor);

    //printf("%s%s %d: All ingredients returned to their rightful places. Moving on...\n", baker->textColor, baker->name, baker->id+1);
    return true;
}

bool checkAllIngredientsRequired(Baker *baker, int start, int end) {
    for (int i = start; i < end; i++) {
        if (baker->currentRecipe.ingredientsAccquired[i] != baker->currentRecipe.ingredientsNeeded[i])
            return false;
    }

    return true;
}

bool checkAllRecipesCooke(bool* recipes, int length)
{
    int i;
    for (i = 0; i < length; i++)
        if (!recipes[i])
            return false;
    return true;
}


void cleanup()
{
    printf("%s", DEFAULT);
    printf("%sInitiating Cleanup\n", DEFAULT);
    int i;
    printf("%sDestroying Semaphores\n", DEFAULT);
    sem_destroy(&mixerSem);
    sem_destroy(&pantrySem);
    sem_destroy(&refridgeratorSem);
    sem_destroy(&bowlSem);
    sem_destroy(&spoonSem);
    sem_destroy(&ovenSem);

    for (i = 0; i < 6; i++) {
        sem_destroy(&pantryIngredients[i]);
    }

    for (i = 0; i < 3; i++) {
        sem_destroy(&fridgeIngredients[i]);
    }
    printf("%sDone\n", DEFAULT);

    //If there's additional things we need to do here, then we'll put them down here

    printf("%sCleanup complete\n", DEFAULT);

}

bool getRamsied()
{
    return rand() % (100 / numBakers) == 1;
}

void gotRamsied(Baker* baker)
{
    int i;
    for (i = 0; i < 9; i++) {
        baker->currentRecipe.ingredientsAccquired[i] = false;
    }

    baker->currentRecipe.allIngredientsAccquired = false;
    baker->currentRecipe.mixed = false;
    baker->currentRecipe.ovened = false;
    baker->currentRecipe.complete = false;

    printf("%s %d has been Ramsied. Now Make it Again!\n", baker->name, baker->id+1);

    if (baker->hasBowl) {
        baker->hasBowl = false;
        releaseResource(&bowlSem, "Bowl", baker->name, baker->id,  baker->textColor);
    }

    if (baker->hasMixer)  {
        baker->hasMixer = false;
        releaseResource(&mixerSem, "Mixer", baker->name, baker->id,  baker->textColor);
    }

    if (baker->hasSpoon) {
        baker->hasSpoon = false;
        releaseResource(&spoonSem, "Spoon", baker->name, baker->id, baker->textColor);
    }

    if (baker->hasOven) {
        baker->hasOven =false;
        releaseResource(&ovenSem, "Oven", baker->name, baker->id, baker->textColor);
    }
}

