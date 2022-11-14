#include "Memory.h"
#include <thread>
#include <iostream>

namespace offsets {
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDE6964;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DFBE54;
	constexpr ::std::ptrdiff_t dwForceAttack = 0x3229CBC;
	constexpr ::std::ptrdiff_t dwClientState = 0x59F194;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;

	constexpr ::std::ptrdiff_t m_iHealth = 0x100;
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_iCrosshairId = 0x11838;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr ::std::ptrdiff_t m_iShotsFired = 0x103E0;
}

struct Vector2 {
	float x = { };
	float y = { };
};

int main() {
	const auto memory = Memory{ "csgo.exe" };
	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");

	std::cout << std::hex << "client.dll -> 0x" << client << std::dec << std::endl;

	auto oldPunch = Vector2{ };

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (!GetAsyncKeyState(VK_SHIFT)) {
			continue;
		}

		auto oldPunch = Vector2{ };
		const auto& localPlayer = memory.Read<std::uintptr_t>(client + offsets::dwLocalPlayer);
		const auto& localHealth = memory.Read<std::int32_t>(localPlayer + offsets::m_iHealth);
		const auto& shotsFired = memory.Read<std::int32_t>(localPlayer + offsets::m_iShotsFired);

		if (shotsFired)
		{
			const auto& clientState = memory.Read<std::uintptr_t>(engine + offsets::dwClientState);
			const auto& viewAngles = memory.Read<Vector2>(clientState + offsets::dwClientState_ViewAngles);

			const auto& aimPunch = memory.Read<Vector2>(localPlayer + offsets::m_aimPunchAngle);

			auto newAngles = Vector2{
				viewAngles.x + oldPunch.x - aimPunch.x * 2.0f,
				viewAngles.y + oldPunch.y - aimPunch.y * 2.0f
			};

			if (newAngles.x > 89.0f) {
				newAngles.x = 89.0f;
			}
			if (newAngles.x < -89.0f) {
				newAngles.x = -89.0f;
			}
			while (newAngles.y > 180.0f) {
				newAngles.y -= 360.0f;
			}
			while (newAngles.y < -180.0f) {
				newAngles.y += 180.0f;
			}

			memory.Write<Vector2>(clientState + offsets::dwClientState_ViewAngles, newAngles);
			oldPunch.x = aimPunch.x * 2.0f;
			oldPunch.y = aimPunch.y * 2.0f;
		}
		else {
			oldPunch.x = oldPunch.y = 0.0f;
		}
		if (!localHealth) {
			continue;
		}

		const auto& crosshairId = memory.Read<std::int32_t>(localPlayer + offsets::m_iCrosshairId);

		if (!crosshairId || crosshairId > 64) {
			continue;
		}

		const auto& player = memory.Read<std::uintptr_t>(client + offsets::dwEntityList + (crosshairId - 1) * 0x10);

		if (!memory.Read<std::int32_t>(player + offsets::m_iHealth)) {
			continue;
		}

		if (memory.Read<std::int32_t>(player + offsets::m_iTeamNum) == memory.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum)) {
			continue;
		}

		memory.Write<std::uintptr_t>(client + offsets::dwForceAttack, 6);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		memory.Write<std::uintptr_t>(client + offsets::dwForceAttack, 4);
	}

	return 0;
}