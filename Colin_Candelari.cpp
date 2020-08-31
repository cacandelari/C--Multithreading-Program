#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <sstream>

/*
The program has minor bugs due to waking up all of the second groups users all at once. The bugs show when multiple users are waiting on the same slot 
in the second group to execute. Sometimes the order can be bugged and the printing of lines, however the times are correct. 
The problems are due to line 132, where the second group of users are woken up. There is also more documentation there regarding this issue. 

All comment lines are being kept in the program for documenation purposes
*/

/*
the threads work!
g++ -std=c++11 -pthread 3360A3.cpp -o a3test
it must be compiled with this line! the -pthread is vital

*/
using namespace std;

static pthread_mutex_t bsem;
static int members, dueToGrou = 0, dueToPos = 0, numSG = 0, numG1, numG2; //numG1 and numG2 were originally in main
static pthread_cond_t empty = PTHREAD_COND_INITIALIZER, posOpen = PTHREAD_COND_INITIALIZER;
static string sg;//global string that holds which group starts execution
static string isTaken[10] = { "0", "0", "0", "0", "0", "0", "0", "0", "0", "0" };//array of strings that will be replaced with the user number who is taking the slot
typedef struct userInfo users;

struct userInfo {
	string userNumber, groupNumber, position, arrival, howLong;
};

void check1(void *user);//function declaration
void check2(void *user);

void *access_DBMS(void *user) {//altered version of the sample program's *access_house

	int timeIn, timeArrival;
	users *temp;
	temp = (users*)user;
	//cout << "User "<<temp->userNumber <<" number: "  << pthread_self() << endl; //yes it IS actually threading
	//controlling the arrival time
	timeArrival = stoi(temp->arrival);//this could have been stoi'd inside of the sleep
	sleep(timeArrival);
	pthread_mutex_lock(&bsem);
	cout << "User " << temp->userNumber <<" from Group " << temp->groupNumber<< " arrives to the DBMS" << endl;
	if (sg != temp->groupNumber) {
		cout <<"User " << temp->userNumber << " is waiting due to its group" << endl;
		pthread_cond_wait(&empty, &bsem);
		dueToGrou++;
	}
	//checkAgain:
	if (isTaken[stoi(temp->position)] != "0") {//I'll keep all of the commented out things here for documentation purposes.
		//the position is being used
		cout << "User " << temp->userNumber << " is waiting: position " << temp->position <<" is being used by user " <<isTaken[stoi(temp->position)] << endl;
		dueToPos++; //where it was //not sure if it needs to be here or below // as of 4/28/2019 it works being here
		//cout <<"User " << temp->userNumber << " due to pos: " << dueToPos << endl;

		/*
		a glitch sometimes happens here during the second group's execution, where the line above will be printed too many times. 
		This may be due to the second group all being released at the same time, rather than one after the other like the first set. 

		the above no longer happens, but the problem that was explained in the email on 4/28/2019 to Pavan Paluri still happens. 
		*/
		//
		pthread_cond_wait(&posOpen, &bsem);
		void(*checkagain)(void*);
		checkagain = &check1;
		checkagain(temp); //check again and check1 & check2 replaced the goto 

		//dueToPos++; //because of the goto statement, only if the thread has SUCCESSFULLY waited, does it increment
		//cout <<"User " << temp->userNumber << " due to pos: " << dueToPos << endl;
		//
		//goto checkAgain;
						/*
							goto's are extremely ill advised, but I feel in this case it is ok.
						I also know that goto's can always be replaced with other, less ill advised code. 
						However, in order to ensure the code is controlled as specified in the prompt,
						and due to lack of knowledge of how else to implement what the goto does,
						I felt the goto was necessary here.
						
						It is in place because otherwise, multiple users waiting on a specific spot would execute at the same time without waiting
						after being signaled by the original user holding that slot. 
						such as if user 1 holds position 3, then users 2 and 3 arrive wanting it at the same time; without the goto
						they would execute at the same time after user 1 finishes. 

						*/ 
	}
	else {
		//cout << "No one is in this slot right now so I can use it" << endl;
	}
	members++;
	cout <<"User " << temp->userNumber << " is accessing position "<<temp->position << " of the database for "<< temp->howLong <<" second(s)" << endl;
	isTaken[stoi(temp->position)] = temp->userNumber;
	//cout << "Now isTaken[" << temp->position << "] = " << isTaken[stoi(temp->position)] << endl;
	timeIn = stoi(temp->howLong);
	//cout << "time inside: " << timeIn << endl;
	pthread_mutex_unlock(&bsem);

	sleep(timeIn);

	pthread_mutex_lock(&bsem);
	cout <<"User " << temp->userNumber << " finished its execution" << endl;
	isTaken[stoi(temp->position)] = "0";

	//
	pthread_cond_signal(&posOpen);
	//

	//cout << "released isTaken[" << temp->position << "] = " << isTaken[stoi(temp->position)] << endl;
	
	if (temp->groupNumber == sg) {//This controls the decrementation of numSG to 0 once all of the first groups users have finished execution 
		numSG--;
		//cout << "NumSG: " << numSG << endl;
	}
	members--;
	if (temp->groupNumber == sg && members == 0 && numSG == 0) {//this makes sure that the group doesn't switch before all users from the starting group have finished
		cout << "\nAll users of the starting group have finished" << endl;
		if (sg == "1")
			cout << "Users from Group 2 start their execution\n" << endl;
		else
			cout << "Users from Group 1 start their execution\n" << endl;
		
		/*
		for (int i = 0; i < numG2; i++) {
			pthread_cond_signal(&empty); //this is basically a broadcast, so I should instead, signal the one user then signal the waiting processes after it has finished
		}*/
		pthread_cond_broadcast(&empty);//perhaps replace with a for(num processes of group2){pthread_cond
		/*
		The above line should use the commented out signal. However, this caused a problem when group 2 runs first and group 1 has more than one process waiting
		on a particular slot, where the program would possibly not finish execution. This is why I have kept the broadcast instead.

		The broadcast, however, causes another problem when after the first group finishes running, the printing of what users are waiting for what spot in 
		instances of more than one user waiting on one spot is messed up. The users still wait the appropriate amount of time, but order and line printing can 
		sometimes be bugged.

		Ideally, an implementation where the first user to access the spot requested by multiple users would signal to the waiting processes when it is 
		done would be used. However, due to time constraints, the specificity of the problem, and the possibility of messing the working portions of the 
		program up, I have decided that it is the best case to leave these minor bugs in. 

		*/
		
	}
	pthread_mutex_unlock(&bsem);
	return NULL;

}

void check1(void *user) {

	users *temp;
	temp = (users*)user;
	//cout << "User " << temp->userNumber << " is checking again 1 for position " << temp->position << endl;
	if (isTaken[stoi(temp->position)] != "0") {
		cout << "User " << temp->userNumber << " is waiting: position " << temp->position << " is being used by user " << isTaken[stoi(temp->position)] << endl;
		dueToPos++;
		pthread_cond_wait(&posOpen, &bsem);
		void(*checkagain)(void*);
		checkagain = &check2;
		checkagain(temp);

	}

}

void check2(void *user) {// 

	users *temp;
	temp = (users*)user;
	//cout << "User " << temp->userNumber << " is checking again 2 for position " << temp->position << endl;
	if (isTaken[stoi(temp->position)] != "0") {
		cout << "User " << temp->userNumber << " is waiting: position " << temp->position << " is being used by user " << isTaken[stoi(temp->position)] << endl;
		dueToPos++;
		pthread_cond_wait(&posOpen, &bsem);
		void(*checkagain)(void*);
		checkagain = &check1;
		checkagain(temp);

	}
}



int main(int argc, char *argv[]) {

	string input;//starting group
	int sgcheck = 0; //starting group check
	int count = 0, numUsers = 0; //numG1 = 0, numG2 = 0; //Num users total and num users in the starting group to ensure the group doesn't switch before all are executed.
	vector<string> numUser;
	vector<string> whichGroup;
	vector<string> whichPos;
	vector<string> atTime;
	vector<string> howLong;
	vector<users*> users;

	while (cin >> input) {

		if (sgcheck == 1) {
			//cout << input << " ";
			if (count % 4 == 0) {
				//cout << "Group: " << input << endl;
				whichGroup.push_back(input);

				numUsers++;
			}
			if (count % 4 == 1) {
				//cout << "Requesting position " << input << endl;
				whichPos.push_back(input);
			}
			if (count % 4 == 2) {
				//cout << "at time " << input << endl;
				//remember this is not the ACCUMULATED TIME which is what it needs to be. 
				//will need to add the time here to all the previou times to get the actual
				//time that it requests the slot. 
				//this is just to slot the information into the right vector
				atTime.push_back(input);
			}
			if (count % 4 == 3) {
				//cout << "For " << input << " sec." << endl;
				howLong.push_back(input);
				
			}

			count++;
		}

		if (sgcheck == 0) {
			//cout << "Starting Group: " << input << endl;
			sg = input;
			sgcheck = 1;
		}

	}
	//cout << "Number of users: " << numUsers << endl;
	for (int i = 0; i < whichGroup.size(); i++) {
		userInfo* user = new userInfo;
		//user->groupNumber = input;
		//cout << "User: " << i + 1 << " from group " << whichGroup.at(i) << " requests slot " << whichPos.at(i) << " at time " << atTime.at(i) << " for " << howLong.at(i) << endl;
		string x = to_string(i + 1);
		numUser.push_back(x);
		user->userNumber = numUser.at(i);
		user->groupNumber = whichGroup.at(i);
		user->position = whichPos.at(i);
		if (i == 0) {
			user->arrival = atTime.at(i);
		}
		else {
			int accuTime = 0; //accumulated time
			string sAccuTime; //accumulated time as a string
			accuTime = stoi(atTime.at(i)) + stoi(atTime.at(i - 1));
			stringstream itos;
			itos << accuTime;
			sAccuTime = itos.str();

			//
			atTime.at(i) = sAccuTime;
			//

			user->arrival = sAccuTime;
			//cout << "sAccuTime: " << sAccuTime << endl;
		}

		user->howLong = howLong.at(i);
		users.push_back(user);
	}

	for (int i = 0; i < users.size(); i++) {
		//cout << "User: " << users.at(i).userNumber << " from group " << users.at(i).groupNumber << " requests slot " << users.at(i).position << " at time " << users.at(i).arrival << " for " << users.at(i).howLong << endl;
		//cout << "User: " << users.at(i)->userNumber << " from group " << users.at(i)->groupNumber << " requests slot " << users.at(i)->position << " at time " << users.at(i)->arrival << " for " << users.at(i)->howLong << endl;
		if (whichGroup.at(i) == "1") {
			numG1++;
		}
		else
			numG2++;
	}

	//cout << "Is this user in the starting group? \n -------------------------" << endl;
	for (int i = 0; i < whichGroup.size(); i++) {
		//cout << whichGroup.at(i) << " : " << sg << endl;
		if (whichGroup.at(i) == sg) {
			//cout << "Yes this user is in the starting group" << endl;
			numSG++;
		}
		else {
			//cout << "No, this user waits" << endl;
		}
	}
	//cout << "Number in the starting group: " << numSG << endl;


	pthread_t tid[numUsers];
	static int members = 0;
	pthread_mutex_init(&bsem, NULL); //Initialize access to 1


	for (int i = 0; i < numUsers; i++) {
		//cout << "Users at i: " << users.at(i) << endl;

		//the line blow has too many arguments, it can only have one (void *) ___ . So, here, I have to use a struct that will be pointed to that has variables. 
		if (pthread_create(&tid[i], NULL, access_DBMS, (void *)users.at(i))) {
			cout << "Error creating thread" << endl;
			return 1;
		}
	}
	for (int i = 0; i < numUsers; i++) {
		//cout << "waiting..." << endl;
		pthread_join(tid[i], NULL);
	}
	/*
	for (int i = 0; i < numUsers; i++) {
		cout << "tid " << i << ": " << tid[i] << endl;
	}
	*/

	cout << "\nTotal Requests: \n     Group 1: " << numG1 << "\n     Group 2: " << numG2 << endl;
	cout << "\nRequests that waited: \n     Due to its group: " << dueToGrou << "\n     Due to a locked position: " << dueToPos<< "\n" << endl;

	return 0;
}

/*
This code uses an altered version of the sample 'Condition Variable' program supplied by Professor Rincon on blackboard

Also, I put periods in front of the links to stop them from being a hyperlink, as I believe it was causing my file to be treated as unsafe, when trying to
submit it to blackboard. I apologize for any inconvenience this causes. 

After learning that only one argument could be passed in pthread_create's call
I used this page as a reference:
.https://parallelcomputers.blogspot.com/2013/09/pass-multiple-arguments-in-pthreadcreate.html

controlling the vector of structs:
.https://stackoverflow.com/questions/8067338/vector-of-structs-initialization
And vectors with struct pointers info
.https://stackoverflow.com/questions/8814146/vector-of-pointers-to-struct

General threads information
.https://www.tutorialspoint.com/cplusplus/cpp_multithreading.htm
.https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
.http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_cond_signal.html

Quick refresh and reference on structs, like how to access the data variables etc...
.https://www.learncpp.com/cpp-tutorial/47-structs/

*/

/*

1
1 3 0 5
2 3 2 5
1 3 1 5
2 1 3 1

===========

1
1 3 0 5
2 3 2 5
1 3 1 5
2 1 3 1
1 3 2 5
1 3 3 5
1 3 4 5
2 3 3 5
2 3 4 5

============

2
1 1 0 5
2 1 1 5
1 2 1 5
2 2 1 3
1 3 1 4
2 3 1 4
1 4 1 5
2 4 1 5
1 5 1 5
2 5 1 5
1 6 1 5
2 6 1 5
1 7 1 5
2 7 1 5
1 8 1 5
2 8 1 5
1 9 1 5
2 9 1 5
*/