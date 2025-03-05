#include <windows.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>
#include "URenderer.h"
#include "Sphere.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include <vector>
enum BallType
{
	Bullet,
	Enemy
};

class UBall
{
public:
	BallType ballType;
	FVector3 Location;
	FVector3 Velocity;
	float Radius;
	float Mass;
	float Rotation;
	float AngularVelocity;
	UBall* next;

	UBall(FVector3 loc, FVector3 vel, float r, float m, BallType balltype)
		: Location(loc), Velocity(vel), Radius(r), Mass(m), ballType(balltype),
		Rotation(0.0f), AngularVelocity(10.0f), next(nullptr) {
	}



	void Update(float deltaTime)
	{
		Location += Velocity * deltaTime;
		Rotation += AngularVelocity * deltaTime;
	}

	static UBall GenerateRandomBall()
	{
		float newRadius = MinRadius + static_cast<float>(rand()) / RAND_MAX * (MaxRadius - MinRadius);
		return UBall(
			FVector3::Random(MinPos, MaxPos),
			FVector3::Random(MinVelocity, MaxVelocity),
			newRadius,
			newRadius * 10,
			BallType::Enemy
		);
	}

private:
	static FVector3 MinPos;
	static FVector3 MaxPos;
	static float MinRadius;
	static float MaxRadius;
	static float MinMass;
	static float MaxMass;
	static FVector3 MinVelocity;
	static FVector3 MaxVelocity;
};

FVector3 UBall::MinPos = FVector3(-1, -1, 0);
FVector3 UBall::MaxPos = FVector3(1, 1, 0);
float UBall::MinRadius = 0.05f;
float UBall::MaxRadius = 0.2f;
float UBall::MinMass = 0.5f;
float UBall::MaxMass = 2.0f;
FVector3 UBall::MinVelocity = FVector3(-0.005f, -0.005f, 0);
FVector3 UBall::MaxVelocity = FVector3(0.005f, 0.005f, 0);

class Player
{
public:
	FVector3 Location;
	float Radius;
	float Rotation;

	Player(FVector3 loc, float radius, float rotation):Location(loc),Radius(radius),Rotation(rotation)
	{
	}

	void Rendering(URenderer* renderer,
		ID3D11Buffer* vertexBufferSphere, UINT numVerticesSphere)
	{
		if (!renderer || !vertexBufferSphere) return;

			FConstants constantData;
			constantData.Offset = Location;
			constantData.Radius = Radius;
			constantData.Rotation = Rotation;

			renderer->UpdateConstant(constantData);
			renderer->RenderPrimitive(vertexBufferSphere, numVerticesSphere);

	}

	void xMove(float distance)
	{
		Location.x += distance;
	}
};

class UBallManager
{
private:
	UBall* head;
	int size;

public://링크드 리스트
	UBallManager() : head(nullptr), size(0) {}
	int getSize()
	{
		return size;
	}

	void addBall(const UBall& ball)
	{
		UBall* newNode = new UBall(ball);
		newNode->next = nullptr;

		if (!head)
		{
			head = newNode;
		}
		else
		{
			UBall* temp = head;
			while (temp && temp->next)
			{
				temp = temp->next;
			}
			if (temp) temp->next = newNode;
		}
		size++;
	}

	void removeBall(UBall* target) {
		if (!head) return;

		if (head == target) {
			UBall* temp = head;
			head = head->next;
			delete temp;
			size--;
			return;
		}

		UBall* current = head;
		while (current->next && current->next != target) {
			current = current->next;
		}

		if (current->next) {
			UBall* temp = current->next;
			current->next = temp->next;
			delete temp;
			size--;
		}
	}

	void removeBallAtIndex(int index) {
		if (index < 0 || index >= size)
			return;

		if (index == 0) {
			UBall* temp = head;
			head = head->next;
			delete temp;
			size--;
			return;
		}

		UBall* current = head;
		int currentIndex = 0;
		while (current && currentIndex < index - 1) {
			current = current->next;
			currentIndex++;
		}

		if (current && current->next) {
			UBall* temp = current->next;
			current->next = temp->next;
			delete temp;
			size--;
		}
	}

	void setBallCount(int n) {
		while (size < n) {
			addBall(UBall::GenerateRandomBall());
		}
		while (size > n) {
			int randomIndex = rand() % size;
			removeBallAtIndex(randomIndex);
		}
	}

	~UBallManager() {
		while (head) {
			UBall* temp = head;
			head = head->next;
			delete temp;
		}
	}

public://충돌처리
	void updateBalls(float deltaTime) {
		UBall* current = head;
		while (current) {
			current->Update(deltaTime);
			current = current->next;
		}
	}

	void checkCollisions() {
		if (size < 2) return;

		UBall* a = head;
		UBall* prevA = nullptr;
		while (a) {
			UBall* prevB = a;
			UBall* b = a->next;
			while (b) {
				/*if (checkCollision(a, b)) {
					resolveElasticCollision(a, b);
				}
				b = b->next;*/
				if (checkCollision(a, b)) {
					if ((a->ballType == BallType::Bullet && b->ballType == BallType::Enemy) ||
						(a->ballType == BallType::Enemy && b->ballType == BallType::Bullet))
					{
						UBall* tempA = a;
						UBall* tempB = b;

						b = b->next;
						if (prevB) prevB->next = b;

						removeBall(tempA);
						removeBall(tempB);

						if (prevA) {
							a = prevA->next;
						}
						else {
							a = head;
						}
						break;
					}
					else {
						resolveElasticCollision(a, b);
					}
				}
				prevB = b;
				b = b->next;
			}
			a = a->next;
		}
	}

	bool checkCollision(UBall* a, UBall* b) {
		float dx = a->Location.x - b->Location.x;
		float dy = a->Location.y - b->Location.y;
		float dz = a->Location.z - b->Location.z;

		float distanceSquared = dx * dx + dy * dy + dz * dz;
		float radiusSum = a->Radius + b->Radius;
		float radiusSumSquared = radiusSum * radiusSum;

		return distanceSquared <= radiusSumSquared;
	}

	void resolveElasticCollision(UBall* a, UBall* b) {

		FVector3 direction = b->Location - a->Location;
		float distance = FVector3::Distance(a->Location, b->Location);
		FVector3 normalDir = direction / distance;
		float depth = a->Radius + b->Radius - distance;
		if (depth > 0.0f)
		{
			float totalMass = a->Mass + b->Mass;
			float ratioA = b->Mass / totalMass;
			float ratioB = a->Mass / totalMass;

			a->Location -= normalDir * depth * ratioA;
			b->Location += normalDir * depth * ratioB;
		}


		float m1 = a->Mass;
		float m2 = b->Mass;
		float v1_x = a->Velocity.x;
		float v1_y = a->Velocity.y;
		float v2_x = b->Velocity.x;
		float v2_y = b->Velocity.y;

		a->Velocity.x = (v1_x * (m1 - m2) + 2 * m2 * v2_x) / (m1 + m2);
		b->Velocity.x = (v2_x * (m2 - m1) + 2 * m1 * v1_x) / (m1 + m2);

		a->Velocity.y = (v1_y * (m1 - m2) + 2 * m2 * v2_y) / (m1 + m2);
		b->Velocity.y = (v2_y * (m2 - m1) + 2 * m1 * v1_y) / (m1 + m2);
	}

	void resolveWallCollision() {
		const float leftBorder = -1.0f;
		const float rightBorder = 1.0f;
		const float bottomBorder = -1.0f;
		const float topBorder = 1.0f;
		
		std::vector<UBall*> ballsToRemove; // 삭제할 Bullet 저장용 벡터

		UBall* current = head;
		while (current) {
			if (current->Location.x - current->Radius < leftBorder) {
				current->Velocity.x *= -1.0f;
				current->Location.x = leftBorder + current->Radius;
			}
			else if (current->Location.x + current->Radius > rightBorder) {
				current->Velocity.x *= -1.0f;
				current->Location.x = rightBorder - current->Radius;
			}

			if (current->Location.y - current->Radius < bottomBorder) {
				current->Velocity.y *= -1.0f;
				current->Location.y = bottomBorder + current->Radius;
			}
			else if (current->Location.y + current->Radius > topBorder) {				
				current->Velocity.y *= -1.0f;
				current->Location.y = topBorder - current->Radius;
				if (current->ballType == Bullet)
				{
					ballsToRemove.push_back(current); // 삭제 목록에 추가
				}
			}

			current = current->next;
		}

		for (UBall* bullet : ballsToRemove) {
			removeBall(bullet);
		}
	}

	void applyGravity(float deltaTime, const FVector3& gravity)
	{

		UBall* current = head;
		while (current)
		{
			current->Velocity += gravity * deltaTime;
			current = current->next;
		}
	}

public://렌더링
	void renderingBalls(URenderer* renderer,
		ID3D11Buffer* vertexBufferSphere, UINT numVerticesSphere)
	{
		if (!renderer || !vertexBufferSphere) return;

		UBall* current = head;

		while (current)
		{
			FConstants constantData;
			constantData.Offset = current->Location;
			constantData.Radius = current->Radius;
			constantData.Rotation = current->Rotation;

			renderer->UpdateConstant(constantData);
			renderer->RenderPrimitive(vertexBufferSphere, numVerticesSphere);

			current = current->next;
		}
	}
};

UBallManager ballManager;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Player player(FVector3(0.0f, -1.0f, 0.0f), 0.05f, 1.0f);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
		{

			float xPos = player.Location.x;
			ballManager.addBall(UBall(FVector3(xPos, -1.0f, 0.0f), FVector3(0.0f, 0.005f, 0.0f), 0.05f, 1.0f, BallType::Bullet));
		}
		if (wParam == VK_LEFT)
		{
			player.xMove(-0.1f);
		}
		if (wParam == VK_RIGHT)
		{
			player.xMove(0.1f);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WCHAR WindowClass[] = L"JungleWindowClass";

	WCHAR Title[] = L"Game Tech Lab";

	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	RegisterClassW(&wndclass);

	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	bool bIsExit = false;

	URenderer renderer;
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();

	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	const int targetFPS = 30;
	const double targetFrameTime = 1000.0 / targetFPS;

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;


	bool isGravity = true;
	int ballCount = 0;

	const FVector3 gravity(0.0f, -0.000049f, 0.0f);

	double totalElapsedTime = 0.0f;
	const double enemySpawnInterval = 1000.0; // 1초마다 적 생성
	double lastEnemySpawnTime = 0.0;

	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}

		}

		// ?? Enemy 생성 (1초마다)
		if (totalElapsedTime - lastEnemySpawnTime >= enemySpawnInterval) {
			printf("[DEBUG] Enemy 생성!\n");

			ballManager.addBall(UBall(
				FVector3(FVector3::Random(FVector3(-1.0f, 1.0f, 0.0f), FVector3(1.0f, 1.0f, 0.0f))),
				FVector3(0.0f, -0.001f, 0.0f),
				0.1f, 2.0f, BallType::Enemy
			));
			lastEnemySpawnTime = totalElapsedTime;
		}

		//ballManager.setBallCount(ballCount);
		//if(isGravity) ballManager.applyGravity(elapsedTime, gravity);
		ballManager.updateBalls(elapsedTime);
		ballManager.checkCollisions();
		ballManager.resolveWallCollision();


		renderer.Prepare();
		renderer.PrepareShader();
		ballManager.renderingBalls(&renderer, vertexBufferSphere, numVerticesSphere);

		player.Rendering(&renderer, vertexBufferSphere, numVerticesSphere);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		//ImGui::Checkbox("Gravity", &isGravity);
		//ImGui::InputInt("##number", &ballCount);
		//if (ballCount < 0)
		//	ballCount = 0;
		//ImGui::SameLine();
		//ImGui::Text("Number of Balls");
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		renderer.SwapBuffer();

		do {
			Sleep(0);
			QueryPerformanceCounter(&endTime);
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

		} while (elapsedTime < targetFrameTime);
		totalElapsedTime += elapsedTime;

	}
	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}