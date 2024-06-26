#pragma once
#include "jip_nvse.h"
#include "CustomEventFilters.h"

namespace AuxTimer
{
	// Code structure copied from JIP's jip_core.h

	constexpr UInt8 AuxTimerVersion = 1;

	struct AuxTimerValue
	{
		double m_timeToCountdown; //original value of time to start counting down.
		double m_timeRemaining; //counts down to 0. If below or at 0, timer is stopped/not running.

		enum Flags : UInt32
		{
			// On by default
			kFlag_RunInMenuMode = 1 << 0,
			kFlag_RunInGameMode = 1 << 1,
			kFlag_CountInSeconds = 1 << 2, // if off, counts in frames
			kFlag_Defaults = kFlag_RunInMenuMode | kFlag_RunInGameMode | kFlag_CountInSeconds,

			// Off by default
			kFlag_AutoRemoveOnLoadAndMainMenu = 1 << 3, // disabled/ignored for savebaked timers.
			kFlag_AutoRestarts = 1 << 4,
			kFlag_IsPaused = 1 << 5,
			kFlag_NotAffectedByTimeMult_InMenuMode = 1 << 6, // if on, seconds-based timers won't be affected by TimeMult in MenuMode.
			kFlag_DontRunWhenGamePaused = 1 << 7, // timer pauses when game is paused (main menu, pause menu, console menu).
			kFlag_RunOnTimerUpdateEvent = 1 << 8, // if on, OnTimerUpdate events will trigger for this timer each frame it counts down.
			kFlag_IgnoreTurbo = 1 << 9, // if on, seconds-based timer will follow the player's turbo speed, instead of being slowed down like everything else.

			// Off by default, hidden and undocumented, for good reason
			kFlag_PendingRemoval = (UInt32)1 << (UInt32)31, // if on, prevent all changes to the timer, for it will be deleted soon.
		};
		UInt32 m_flags;

		AuxTimerValue() : m_timeToCountdown(0.0), m_timeRemaining(0.0), m_flags(kFlag_Defaults) { }
		AuxTimerValue(double timeToCountdown, UInt32 flags = kFlag_Defaults) :
			m_timeToCountdown(timeToCountdown), m_timeRemaining(timeToCountdown), m_flags(flags) { }
		AuxTimerValue(double timeToCountdown, double timeRemaining, UInt32 flags = kFlag_Defaults) :
			m_timeToCountdown(timeToCountdown), m_timeRemaining(timeRemaining), m_flags(flags) { }

		[[nodiscard]] double GetTimeLeft() const { return m_timeRemaining; }

		void SetTimeToCountdown(double time) {
			m_timeToCountdown = time;
			m_timeRemaining = time;
		}
		void RestartTimer() {
			m_timeRemaining = m_timeToCountdown;
			m_flags &= ~kFlag_IsPaused;
		}
		[[nodiscard]] double GetTimeToCountdown() const {
			return m_timeToCountdown;
		}
		[[nodiscard]] double GetTimeRemaining() const {
			return m_timeRemaining;
		}
		[[nodiscard]] double GetTimeElapsed() const {
			return m_timeToCountdown - m_timeRemaining;
		}
		[[nodiscard]] bool IsPendingRemoval() const {
			return (m_flags & kFlag_PendingRemoval) != 0;
		}

		void WriteValData() const {
			WriteRecord64(&m_timeToCountdown);
			WriteRecord64(&m_timeRemaining);
			WriteRecord32(m_flags);
		}
	};
	static_assert(sizeof(AuxTimerValue) == 24);

	using NameString = char*;

	using AuxTimerVarsMap = UnorderedMap<NameString, AuxTimerValue>;
	using AuxTimerOwnersMap = UnorderedMap<RefID, AuxTimerVarsMap>;
	using AuxTimerModsMap = UnorderedMap<ModID, AuxTimerOwnersMap>;
	// Ensure thread safety when modifying these globals!
	extern AuxTimerModsMap s_auxTimerMapArraysPerm, s_auxTimerMapArraysTemp;

	struct AuxTimerMapInfo
	{
		UInt32		ownerID;
		UInt32		modIndex;
		char		*varName;
		bool		isPerm;

		AuxTimerMapInfo(TESForm* form, TESObjectREFR* thisObj, const Script* scriptObj, char* pVarName)
		{
			if (!pVarName[0])
			{
				ownerID = 0;
				return;
			}
			ownerID = GetSubjectID(form, thisObj);
			if (ownerID)
			{
				varName = pVarName;
				isPerm = (varName[0] != '*');
				// If an AuxTimer func is called from console, will have 0xFF aka Public mod index.
				modIndex = (varName[!isPerm] == '_') ? 0xFF : scriptObj->GetOverridingModIdx();
			}
		}

		AuxTimerMapInfo(TESForm* form, TESObjectREFR* thisObj, const Script* scriptObj, UInt8 type) :
			ownerID(GetSubjectID(form, thisObj)), varName(nullptr)
		{
			if (ownerID)
			{
				isPerm = !(type & 1);
				modIndex = (type > 1) ? 0xFF : scriptObj->GetOverridingModIdx();
			}
		}

		[[nodiscard]] AuxTimerModsMap& ModsMap() const {
			return isPerm ? s_auxTimerMapArraysPerm : s_auxTimerMapArraysTemp;
		}

		[[nodiscard]] bool IsPublic() const {
			return modIndex == 0xFF;
		}
	};

	AuxTimerValue* __fastcall GetTimerValue(const AuxTimerMapInfo& varInfo, bool createIfNotFound);

	namespace Impl
	{
		void DoCountdown(double vatsTimeMult, bool isMenuMode, bool isTemp);
	}
	void DoCountdown(double vatsTimeMult, bool isMenuMode);
	void HandleAutoRemoveTempTimers();

	struct AuxTimerPendingRemoval
	{
		UInt32		modIndex;
		UInt32		ownerID;
		std::string	varName;
	};

	extern std::vector<AuxTimerPendingRemoval> g_auxTimersToRemovePerm, g_auxTimersToRemoveTemp;
	namespace Impl
	{
		void RemovePendingTimers(bool clearTemp);
	}
	void RemovePendingTimers();
}