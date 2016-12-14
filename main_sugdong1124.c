#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <time.h>
#include "./block.h"

#define LEFT 75
#define RIGHT 77
#define SPACE 32
#define UP 72

/* Size of game board */
#define GBOARD_WIDTH 10
#define GBOARD_HEIGHT 20

/* Starting point of game board*/
#define GBOARD_ORIGIN_X 4
#define GBOARD_ORIGIN_Y 2

//게임 전체적 스피드(키 입력)
#define D_SLEEPSPEED 40
//콘솔에서의 색상 개수
#define D_COLORNUM 15
//NPC블럭 생성 시 피봇 개수
#define D_PIVOTCNTOFNEWBLOCK 3
//NPC블럭생성 스피드 값
#define D_NEWBLOCKSPEED 0.1
//콤보유효시간계산 스피드 값
#define D_NEWCOMBOSPEED 0.5
//콤보유효시간 주기
#define D_NEWCOMBOTIMECYCLE 2
//폭탄 인덱스
#define D_BOMBINDEX 9

//게임보드맵
static int gameBoardInfo[GBOARD_HEIGHT + 1][GBOARD_WIDTH + 2];
//PC블럭 좌표
static COORD curPos;
//스코어
static int g_nScore = 190;
//PC블럭의 값
static int g_userNum = 0;
//NPC블럭속도 관련(스피드값을 더한 값)
static float g_newBlockCnt = 1.0f;
//NPC블럭속도 관련(생성 주기)
static int g_newBlockNum = 1;
//NPC블럭의 최고 높이를 저장함
static int g_BlockHeight = GBOARD_HEIGHT;
//충돌 여부를 저장. PC블록 재생성에 사용
static bool g_IsCollision = false;
//콤보 카운트
static int g_nComboCnt;
//콤보속도 관련(스피드값을 더한 값)
static float g_newComboTime = 1.0f;
//PC블록과 NPC블록의 연산을 결정함.
static char g_Operation = '-';
//NPC블록 연결리스트의 헤드.
static stMapBlock* g_mapBlockHead = NULL;
//저장 블록
static int g_nSaveBlock = -1;

int DetectCollision(int posX, int posY);
void AddBlockToBoard();
void RedrawScore();
void RedrawCombo();
void AddCombo();
void FallingBlock();
void DrawGameBoard();

//콘솔상의 커서 위치를 설정함.
void SetCurrentCursorPos(int x, int y)
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

//PC블럭의 좌표를 설정함. 
void SetUserPos(int x, int y) {
	curPos.X = x;
	curPos.Y = y;
}

//커서좌표를 반환한다.
COORD GetCurrentCursorPos(void)
{
	COORD curPoint;
	CONSOLE_SCREEN_BUFFER_INFO curInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curPoint.X = curInfo.dwCursorPosition.X;
	curPoint.Y = curInfo.dwCursorPosition.Y;

	return curPoint;
}

//PC블록을 보여줌
void ShowBlock()
{
	SetCurrentCursorPos(curPos.X, curPos.Y);
	printf("%s", BlockNum[g_userNum]);
	SetCurrentCursorPos(curPos.X, curPos.Y);
}

//PC블록을 지워줌
void DeleteBlock()
{
	SetCurrentCursorPos(curPos.X, curPos.Y);
	printf(" ");
	SetCurrentCursorPos(curPos.X, curPos.Y);
}

//처음에 커서깜빡이는것을 없애줌.
void RemoveCursor(void)
{
	CONSOLE_CURSOR_INFO curInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curInfo.bVisible = 0;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

//게임종료조건을 검사함.
int IsGameOver(void)
{
	if (g_BlockHeight < GBOARD_ORIGIN_Y) {
		return 1;
	}
	return 0;
}

//맵에 NPC블록들을 다시 그림
void RedrawBlocks(void)
{
	int x, y;
	int cursX, cursY;
	stMapBlock* temp = NULL;

	for (y = 0; y < GBOARD_HEIGHT; y++)
	{
		for (x = 1; x < GBOARD_WIDTH + 1; x++)
		{
			cursX = x * 2 + GBOARD_ORIGIN_X;
			cursY = y + GBOARD_ORIGIN_Y;

			SetCurrentCursorPos(cursX, cursY);

			if (gameBoardInfo[y][x] == 0)
			{
				printf("  ");
			}
		}
	}
	temp = g_mapBlockHead;
	while (1) {
		if (temp == NULL)
			break;

		cursX = temp->nXPos * 2 + GBOARD_ORIGIN_X;
		cursY = temp->nYPos + GBOARD_ORIGIN_Y;
		SetCurrentCursorPos(cursX, cursY);

		//커서에 색상을 적용함.
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), temp->nColor);
		for (int x = 0; x<temp->nWidth; x++)
			printf("%s", BlockNum[temp->nValue - 1]);
		temp = temp->pTail;
	}
	//커서에 다시 흰색으로 적용함.
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

//스코어를 증가시킴.
void AddScore(int nScore) {
	g_nScore += nScore;
	RedrawScore();

	AddCombo();
}

//증가시킨 스코어를 다시 그린다.
void RedrawScore() {
	int before_x, before_y;
	before_x = curPos.X;
	before_y = curPos.Y;

	SetCurrentCursorPos(30, 5);
	printf("SCORE: %d", g_nScore);
	SetCurrentCursorPos(before_x, before_y);
}

//게임의 연산 방식을 결정한다.
void GameOperation(char operation)
{
	if (g_nScore >= 200) {
		g_Operation = '+';
		DrawGameBoard();
	}
	else if (g_nScore >= 100) {
		g_Operation = 'x';
		DrawGameBoard();
	}
	else if (g_nScore >= 0) {
		g_Operation = '-';
		DrawGameBoard();
	}
}

//연산 방식을 다시 그린다.
void RedrawOperation() {
	int before_x, before_y;
	before_x = curPos.X;
	before_y = curPos.Y;

	SetCurrentCursorPos(30, 6);
	printf("Operation: %c", g_Operation);
	SetCurrentCursorPos(before_x, before_y);


}

//콤보를 증가시킴.
void AddCombo() {
	g_newComboTime = 0.0f;
	g_nComboCnt++;
	if (g_nComboCnt % 3 == 0) {
		g_userNum = D_BOMBINDEX;
	}
	RedrawCombo();
}

//증가시킨 콤보를 다시 그린다.
void RedrawCombo() {
	int before_x, before_y;
	before_x = curPos.X;
	before_y = curPos.Y;

	if (g_nComboCnt != 0) {
		SetCurrentCursorPos(30, 2);
		printf("COMBO : %d", g_nComboCnt);
		SetCurrentCursorPos(before_x, before_y);
	}
	else {
		SetCurrentCursorPos(30, 2);
		printf("            ");
		SetCurrentCursorPos(before_x, before_y);
	}

}

void SaveBlock(int value) {
	if (g_nSaveBlock == -1)
		g_nSaveBlock = value;
}
void RedrawSaveBlock() {
	int before_x, before_y;
	before_x = curPos.X;
	before_y = curPos.Y;

	SetCurrentCursorPos(30, 14);
	if (g_nSaveBlock != -1) {
		printf("Save : %d      ", g_nSaveBlock);
	}
	else {
		printf("Save : EMPTY");
	}
	SetCurrentCursorPos(before_x, before_y);
}

void ChangeBlock() {
	if (g_nSaveBlock != -1) {
		int beforeValue = g_userNum;
		g_userNum = g_nSaveBlock - 1;
		g_nSaveBlock = beforeValue + 1;
		RedrawSaveBlock();
		DeleteBlock();
		ShowBlock();
	}
}



//충돌했을 때 지정된 연산방식에 따라 연산을 수행한다.
void CalcCollisionBlock(stMapBlock* obj, char operation) {
	int beforeValue = g_userNum;

	if (g_userNum == D_BOMBINDEX) {
		g_userNum = obj->nValue - 1;
		obj->nValue = 0;
		return;
	}

	switch (operation) {
	case '-':
		g_userNum = obj->nValue - 1;
		obj->nValue = abs(obj->nValue -= (beforeValue + 1));
		for (int x = 0; x < obj->nWidth; x++) {
			gameBoardInfo[obj->nYPos][x + obj->nXPos] = abs(gameBoardInfo[obj->nYPos][x + obj->nXPos] - (beforeValue + 1));
		}
		break;

	case '+':
		g_userNum = obj->nValue - 1;
		obj->nValue = (obj->nValue += (beforeValue + 1)) % 10;
		for (int x = 0; x < obj->nWidth; x++) {
			gameBoardInfo[obj->nYPos][x + obj->nXPos] = (gameBoardInfo[obj->nYPos][x + obj->nXPos] + (beforeValue + 1)) % 10;
		}

		break;

	case 'x':
		g_userNum = obj->nValue - 1;
		if (obj->nValue == beforeValue + 1)
			beforeValue = -1;
		obj->nValue = (obj->nValue *= (beforeValue + 1)) % 10;
		for (int x = 0; x < obj->nWidth; x++) {
			gameBoardInfo[obj->nYPos][x + obj->nXPos] = (gameBoardInfo[obj->nYPos][x + obj->nXPos] * (beforeValue + 1)) % 10;
		}

		break;

		//case '/':
		//	g_userNum = obj->nValue - 1;
		//	obj->nValue = obj->nValue %= (beforeValue + 1);
		//	for (int x = 0; x < obj->nWidth; x++) {
		//		gameBoardInfo[obj->nYPos][x + obj->nXPos] = gameBoardInfo[obj->nYPos][x + obj->nXPos] % (beforeValue + 1);
		//	}

		break;

	default:

		break;
	}
}

//PC블록과 NPC블록이 겹치는 위치에 있는지 결과를 반환한다.
bool IsRangeOfBlock(stMapBlock* obj, int arrX, int arrY) {
	if (obj->nYPos == arrY && obj->nXPos <= arrX && obj->nXPos + obj->nWidth > arrX) {
		return 1;
	}

	return 0;
}

//제거된 NPC블록을 메모리를 해제한다.
void FreeRemoveBlock(stMapBlock* temp, stMapBlock* before) {
	stMapBlock* del = temp;

	for (int x = temp->nXPos; x < temp->nXPos + temp->nWidth; x++) {
		gameBoardInfo[temp->nYPos][x] = 0;
	}
	temp = temp->pTail;
	if (g_mapBlockHead == del) {
		if (g_mapBlockHead->pTail != NULL) {
			g_mapBlockHead = g_mapBlockHead->pTail;
		}
		else {
			g_mapBlockHead = NULL;
		}
	}
	else {
		before->pTail = temp;
	}
	free(del);
	del = NULL;
}

//PC블록과 NPC블록의 충돌을 검사한다.
int DetectCollision(int posX, int posY) {
	//커서좌표를 기준으로한다.

	int arrX = (posX - GBOARD_ORIGIN_X) / 2;
	int arrY = (posY - GBOARD_ORIGIN_Y);
	if (arrY >= GBOARD_HEIGHT) {
		g_IsCollision = true;
		g_newBlockCnt = g_newBlockNum;
		AddBlockToBoard();
		return 0;
	}
	if (arrX == 0 || arrX == GBOARD_WIDTH + 1)
		return 0;

	int beforeValue = 0;
	stMapBlock* pivot = NULL;
	stMapBlock* del = NULL;
	stMapBlock* before = NULL;

	if (gameBoardInfo[arrY][arrX] != 0) {
		pivot = g_mapBlockHead;

		while (1) {
			if (pivot == NULL)
				break;
			if (IsRangeOfBlock(pivot, arrX, arrY)) {

				beforeValue = pivot->nValue;

				CalcCollisionBlock(pivot, g_Operation);

				if (pivot->nValue == 0) {
					AddScore(10);
					FreeRemoveBlock(pivot, before);
					FallingBlock();

					SaveBlock(beforeValue);
					RedrawSaveBlock();
				}
				RedrawBlocks();
				RedrawOperation();
				GameOperation(g_Operation);

				break;
			}
			before = pivot;
			pivot = pivot->pTail;
		}
		SetUserPos(rand() % GBOARD_WIDTH * 2 + GBOARD_ORIGIN_X * 2 - 2, 2);
		ShowBlock();
		g_IsCollision = true;
		return 0;
	}

	g_IsCollision = false;
	return 1;
}

//NPC블록을 한 라인씩 위로 올린다.
void FloatingBlock() {
	for (int y = 1; y < GBOARD_HEIGHT - 1; y++) {
		for (int x = 1; x < GBOARD_WIDTH + 1; x++) {
			if (gameBoardInfo[y + 1][x] != 0) {
				gameBoardInfo[y][x] = gameBoardInfo[y + 1][x];
				gameBoardInfo[y + 1][x] = 0;
			}
		}
	}

	if (g_mapBlockHead != NULL) {
		stMapBlock* temp = g_mapBlockHead;
		while (1) {
			temp->nYPos--;

			if (g_BlockHeight > temp->nYPos) {
				g_BlockHeight = temp->nYPos;
			}

			if (temp->pTail == NULL)
				break;
			temp = temp->pTail;
		}
	}
}

//NPC블록이 그 위치에 존재하는지 여부를 반환한다.
bool DetectCollisionBtwMapBlock(int XPos, int YPos, int Width) {
	int nEmptyCnt = 0;

	for (int i = XPos; i < XPos + Width; i++) {
		if (gameBoardInfo[YPos][i] == 0)
			nEmptyCnt++;
	}
	if (nEmptyCnt == Width)
		return 0;

	return 1;
}

//NPC블록을 쌓기 위해 밑에 여백이 있으면 떨어뜨린다.
void FallingBlock() {
	stMapBlock* obj = g_mapBlockHead;
	int nEmptyCnt = 0;
	bool IsFalling = false;

	while (obj != NULL) {
		while (!DetectCollisionBtwMapBlock(obj->nXPos, obj->nYPos + 1, obj->nWidth)) {
			for (int i = obj->nXPos; i < obj->nXPos + obj->nWidth; i++) {
				gameBoardInfo[obj->nYPos + 1][i] = gameBoardInfo[obj->nYPos][i];
				gameBoardInfo[obj->nYPos][i] = 0;
			}
			obj->nYPos++;
			FallingBlock();
		}
		obj = obj->pTail;
	}
}

//새로운 NPC블록 라인을 생성한다.
void AddBlockToBoard() {
	int newBlockNum = 0;
	int newBlockStartX = 0;
	int newBlockW = 0;
	int newBlockColor = 0;
	int LineLength = 0;
	stMapBlock* temp;

	if (g_newBlockCnt >= g_newBlockNum) {
		g_newBlockCnt = 0;

		FloatingBlock();

		//맨아래줄 생성.

		int nCount = 0;
		int pivot[3] = { 0, };
		int j = 0;

		//피봇에 랜덤값 지정.
		for (int i = 0; i < D_PIVOTCNTOFNEWBLOCK; i++) {
			pivot[i] = rand() % GBOARD_WIDTH;
		}

		//피봇을 정렬
		int smaller = 0;
		for (int i = 0; i < D_PIVOTCNTOFNEWBLOCK; i++) {
			for (j = i + 1; j < D_PIVOTCNTOFNEWBLOCK; j++) {
				if (pivot[i] == pivot[j]) {
					pivot[i] = -1;
				}
				if (pivot[i] > pivot[j]) {
					int temp = pivot[i];
					pivot[i] = pivot[j];
					pivot[j] = temp;
				}
			}
		}

		//지정한 피봇 개수만큼 블록을 생성
		while (nCount < D_PIVOTCNTOFNEWBLOCK) {
			if (pivot[nCount] == -1) {
				nCount++;
				continue;
			}

			int widthRange = GBOARD_WIDTH;
			int smaller = GBOARD_WIDTH;

			//다음 피봇과의 거리를 잰다.
			if (nCount != D_PIVOTCNTOFNEWBLOCK - 1) {
				widthRange = pivot[nCount + 1] - pivot[nCount];
			}

			//값들을 랜덤으로 지정.
			newBlockNum = rand() % (GBOARD_WIDTH - 1) + 1;
			newBlockStartX = pivot[nCount];
			newBlockW = rand() % widthRange + 1;
			newBlockColor = rand() % D_COLORNUM + 1;

			if (g_mapBlockHead == NULL) {
				g_mapBlockHead = (stMapBlock*)malloc(sizeof(stMapBlock));
				g_mapBlockHead->nValue = newBlockNum;
				g_mapBlockHead->nWidth = newBlockW;
				g_mapBlockHead->nXPos = newBlockStartX + 1;
				g_mapBlockHead->nYPos = GBOARD_HEIGHT - 1;
				g_mapBlockHead->nColor = newBlockColor;
				g_mapBlockHead->pTail = NULL;

				temp = g_mapBlockHead;
			}
			else {
				temp = g_mapBlockHead;
				while (1) {
					if (temp->pTail == NULL)
						break;
					temp = temp->pTail;
				}

				stMapBlock* block = (stMapBlock*)malloc(sizeof(stMapBlock));
				block->nValue = newBlockNum;
				block->nWidth = newBlockW;
				block->nXPos = newBlockStartX + 1;
				block->nYPos = GBOARD_HEIGHT - 1;
				block->nColor = newBlockColor;
				block->pTail = NULL;

				temp->pTail = block;
				temp = block;
			}

			//설정된 값들을 게임보드에 넣음.
			for (int x = 1; x < newBlockW + 1; x++) {
				if (x + newBlockStartX > GBOARD_WIDTH) {
					temp->nWidth = GBOARD_WIDTH - newBlockStartX;
					break;
				}
				gameBoardInfo[GBOARD_HEIGHT - 1][x + newBlockStartX] = newBlockNum;
			}
			nCount++;
		}

		FallingBlock();

		RedrawBlocks();

		DetectCollision(curPos.X, curPos.Y);
	}
}

//PC블록을 한 칸 내린다.
int BlockDown() {
	if (!DetectCollision(curPos.X, curPos.Y + 1)) {
		return 0;
	}

	DeleteBlock();
	Sleep(D_SLEEPSPEED);
	SetCurrentCursorPos(curPos.X, ++curPos.Y);
	ShowBlock();

	return 1;
}

//PC블록을 왼쪽으로 한 칸 이동한다.
void ShiftLeft() {
	/*if (g_IsCollision)
	return;*/

	if (!DetectCollision(curPos.X - 2, curPos.Y)) {
		/*DeleteBlock();
		SetUserPos(GBOARD_ORIGIN_X + GBOARD_WIDTH * 2, curPos.Y);
		ShowBlock();*/
		return;
	}

	DeleteBlock();
	SetCurrentCursorPos(curPos.X -= 2, curPos.Y);
	ShowBlock();
}

//PC블록을 오른쪽으로 한 칸 이동한다.
void ShiftRight() {
	/*if (g_IsCollision)
	return; */

	if (!DetectCollision(curPos.X + 2, curPos.Y)) {
		/*DeleteBlock();
		SetUserPos(GBOARD_ORIGIN_X + 2, curPos.Y);
		ShowBlock();*/
		return;
	}

	DeleteBlock();
	SetCurrentCursorPos(curPos.X += 2, curPos.Y);
	ShowBlock();
}

//PC블록을 아래로 떨어뜨린다.
void SpaceDown() {
	DeleteBlock();
	while (1) {
		if (!DetectCollision(curPos.X, curPos.Y + 1)) {
			break;
		}
		curPos.Y++;
	}
}

//키입력을 처리한다.
void ProcessKeyInput() {
	int key;
	for (int i = 0; i<20; i++)
	{
		if (_kbhit() != 0)
		{
			key = _getch();
			switch (key)
			{
			case LEFT:
				ShiftLeft();
				break;

			case RIGHT:
				ShiftRight();
				break;

			case SPACE:
				SpaceDown();
				break;

			case UP:
				ChangeBlock();
				break;

			default:

				break;

			}
		}
		Sleep(D_SLEEPSPEED);
	}
	g_newBlockCnt += D_NEWBLOCKSPEED;
	g_newComboTime += D_NEWCOMBOSPEED;
	if (g_newComboTime > D_NEWCOMBOTIMECYCLE) {
		g_nComboCnt = 0;
		g_newComboTime = 0.0f;
		RedrawCombo();
	}
}

//최초에 게임보드(맵)를 그린다.
void DrawGameBoard()
{
	int x, y;
	char* boardCharacter;

	switch (g_Operation) {
	case '-':
		boardCharacter = "─";
		break;

	case '+':
		boardCharacter = "┼";
		break;

	case 'x':
		boardCharacter = "Х";
		break;
	}
	for (y = 0; y <= GBOARD_HEIGHT; y++)
	{
		SetCurrentCursorPos(GBOARD_ORIGIN_X, GBOARD_ORIGIN_Y + y);
		if (y == GBOARD_HEIGHT)
			printf(boardCharacter);//┼Х─
		else
			printf(boardCharacter);

		SetCurrentCursorPos(GBOARD_WIDTH + 6 + GBOARD_WIDTH, GBOARD_ORIGIN_Y + y);
		if (y == GBOARD_HEIGHT)
			printf(boardCharacter);
		else
			printf(boardCharacter);
	}

	SetCurrentCursorPos(GBOARD_ORIGIN_X + 2, GBOARD_HEIGHT + GBOARD_ORIGIN_Y);

	for (x = 0; x < GBOARD_WIDTH; x++)
	{
		printf(boardCharacter);
	}

	for (y = 0; y <= GBOARD_HEIGHT; y++)
	{
		gameBoardInfo[y][0] = 1;
		gameBoardInfo[y][GBOARD_WIDTH + 1] = 1;
	}
	for (x = 0; x < GBOARD_WIDTH + 2; x++)
	{
		gameBoardInfo[GBOARD_HEIGHT][x] = 1;
	}
	RedrawCombo();
	RedrawScore();
	RedrawOperation();

	int before_x, before_y;
	before_x = curPos.X;
	before_y = curPos.Y;

	SetCurrentCursorPos(30, 7);
	printf("←→      : MOVE");
	SetCurrentCursorPos(30, 8);
	printf("SPACE BAR : DOWN");
	SetCurrentCursorPos(before_x, before_y);
}



int main()
{
	DrawGameBoard();
	RedrawSaveBlock();
	SetUserPos(rand() % GBOARD_WIDTH * 2 + GBOARD_ORIGIN_X * 2 - 2, 2);
	g_userNum = rand() % 9;

	while (1)
	{
		srand(time(NULL));

		RemoveCursor();


		SetCurrentCursorPos(curPos.X, curPos.Y);
		ShowBlock();

		srand(time(NULL));

		if (IsGameOver()) {
			break;
		}

		while (1)
		{

			AddBlockToBoard();
			BlockDown();
			ProcessKeyInput();
			if (g_IsCollision) {
				break;
			}
		}
	}

	SetCurrentCursorPos(10, 2);
	puts("Game Over");

	return 0;
}