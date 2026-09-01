// core_achievements_native.cpp -- native SIDL achievement window.
// ================================================================
// Folded into AoTv4's shared-detour architecture. The upstream reference was a
// standalone, header-only module that installed its OWN detours on
// CEverQuest::dsp_chat and CEverQuest::InterpretCmd -- but our dll already owns
// both of those hooks (core_spellwindow.cpp / core_bazaar.h), so a second hook on
// the same address would crash startup. Here the chat/command detours are stripped;
// the plain functions (NativeAchievementParseTransport / HandleLocalCommand /
// RewriteCommand) are instead CALLED from our existing detours. Only the UI-reset
// hook (CDisplay::CleanGameUI / ReloadUI) is installed locally (no conflict).
//
// Multi-TU note: the reference used file-scope `static` funcs + globals, which would
// become separate copies per translation unit. Refactored so the single definition
// lives in this .cpp; the 4 cross-TU entry points are declared (extern) in the .h.

#include "MQ2Main.h"
#include "core_advloot.h"   // AdvLootOnUiReset() -- this detour owns the UI-teardown callback
#include "core_lostwindow.h" // LostWindowOnUiReset() -- likewise
#include "core_autoskill.h"  // AutoSkillOnUiReset() -- likewise
#include "core_dungeon.h"    // DungeonOnUiReset() -- likewise
#include "core_difficulty.h" // DifficultyOnUiReset() -- likewise
#include "core_fctwindow.h"  // FctWindowOnUiReset() -- likewise
#include "core_meter.h"      // MeterWindowOnUiReset() -- likewise
#include "core_fellowship.h" // FellowshipOnUiReset() -- likewise
#include "core_travel.h"     // TravelOnUiReset()     -- likewise
#include "core_floatingtext.h" // FloatingTextOnUiReset() -- releases D3D fonts, keeps the device hooks
#include "core_allaclone.h"  // AllacloneOnUiReset() -- likewise
#include "core_aotmenu.h"       // AoTMenuOnUiReset() -- likewise
#include "core_spellchoice_native.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core_achievements_native.h"

static void NativeAchievementsSendCommand(const char* command);
static void NativeAchievementEnsureWindow(bool show);

static void NativeAchievementsTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) {
		sprintf_s(path, "%s\\native_achievements.log", gszEQPath);
	}
	else {
		strcpy_s(path, "native_achievements.log");
	}

	FILE* file = nullptr;
	if (fopen_s(&file, path, "a") || !file) {
		return;
	}

	SYSTEMTIME now;
	GetLocalTime(&now);
	fprintf(
		file,
		"[%04d-%02d-%02d %02d:%02d:%02d] ",
		now.wYear,
		now.wMonth,
		now.wDay,
		now.wHour,
		now.wMinute,
		now.wSecond
	);

	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);

	fprintf(file, "\n");
	fclose(file);
}

static bool NativeStartsWith(const char* value, const char* prefix)
{
	return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
}

static std::string NativeGetPairValue(const std::string& payload, const char* key)
{
	const std::string prefix = std::string(key) + "=";
	size_t pos = 0;

	while (pos < payload.size()) {
		const size_t end = payload.find('|', pos);
		const size_t count = end == std::string::npos ? std::string::npos : end - pos;
		const std::string part = payload.substr(pos, count);

		if (part.compare(0, prefix.size(), prefix) == 0) {
			return part.substr(prefix.size());
		}

		if (end == std::string::npos) {
			break;
		}

		pos = end + 1;
	}

	return std::string();
}

static int NativeToInt(const std::string& value, int fallback = 0)
{
	if (value.empty()) {
		return fallback;
	}

	return atoi(value.c_str());
}

static bool NativeToBool(const std::string& value)
{
	return NativeToInt(value) != 0 || value == "true" || value == "on";
}

static bool NativeCommandMatch(const char* line, const char* command, const char** arguments)
{
	if (!line || !command) {
		return false;
	}

	const size_t command_length = strlen(command);
	if (_strnicmp(line, command, command_length) != 0) {
		return false;
	}

	const char next = line[command_length];
	if (next != 0 && next != ' ' && next != '\t') {
		return false;
	}

	if (arguments) {
		const char* current = line + command_length;
		while (*current == ' ' || *current == '\t') {
			++current;
		}
		*arguments = current;
	}

	return true;
}

struct NativeAchievementCategoryRow
{
	int id = 0;
	int parent_id = 0;
	int total = 0;
	int completed = 0;
	int points = 0;
	std::string name = "Category";
};

struct NativeAchievementRow
{
	int id = 0;
	int category_id = 0;
	int points = 0;
	bool completed = false;
	std::string name = "Achievement";
	std::string description;
};

struct NativeAchievementObjectiveRow
{
	int id = 0;
	int current = 0;
	int required = 0;
	bool completed = false;
	std::string name = "Objective";
};

struct NativeAchievementRewardRow
{
	uint64_t definition_id = 0;
	int reward_id = 0;
	int amount = 0;
	bool auto_claim = false;
	std::string type;
	std::string tier;
	std::string name;
};

class NativeAchievementWnd : public CCustomWnd
{
public:
	NativeAchievementWnd() : CCustomWnd((char*)"NativeAchievementWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAchievementWnd);

		SummaryLabel = GetChildItem("NAW_SummaryLabel");
		CategoryList = (CListWnd*)GetChildItem("NAW_CategoryList");
		AchievementList = (CListWnd*)GetChildItem("NAW_AchievementList");
		DetailTitleLabel = GetChildItem("NAW_DetailTitleLabel");
		DetailDescriptionLabel = GetChildItem("NAW_DetailDescriptionLabel");
		ObjectiveList = (CListWnd*)GetChildItem("NAW_ObjectiveList");
		RewardList = (CListWnd*)GetChildItem("NAW_RewardList");
		StatusLabel = GetChildItem("NAW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("NAW_RefreshButton");
		CheckButton = (CButtonWnd*)GetChildItem("NAW_CheckButton");

		Layout();
		SetStatus("Open with /achievement, /ach, #achievement, or #ach.");
		RefreshRows();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)RefreshButton) {
				char command[128];
				sprintf_s(command, "/say #ach native refresh %d %d", SelectedCategoryID, SelectedAchievementID);
				NativeAchievementsSendCommand(command);
				SetStatus("Refreshing achievements...");
				return 1;
			}

			if (pWnd == (CXWnd*)CheckButton) {
				char command[128];
				sprintf_s(command, "/say #ach native check %d %d", SelectedCategoryID, SelectedAchievementID);
				NativeAchievementsSendCommand(command);
				SetStatus("Checking automatic achievements...");
				return 1;
			}

			if (pWnd == (CXWnd*)CategoryList) {
				NativeAchievementCategoryRow* category = GetSelectedCategory();
				if (category && category->id > 0 && category->id != SelectedCategoryID) {
					char command[128];
					sprintf_s(command, "/say #ach native category %d", category->id);
					NativeAchievementsSendCommand(command);
					SetStatus("Loading achievement category...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)AchievementList) {
				NativeAchievementRow* achievement = GetSelectedAchievement();
				if (achievement && achievement->id > 0 && achievement->id != SelectedAchievementID) {
					char command[128];
					sprintf_s(command, "/say #ach native detail %d %d", achievement->id, SelectedCategoryID);
					NativeAchievementsSendCommand(command);
					SetStatus("Loading achievement detail...");
				}
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
		// Achievement resize and polish are handled by SIDL placement.
	}

	void RefreshRows();
	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text ? text : "");
	}

private:
	NativeAchievementCategoryRow* GetSelectedCategory();
	NativeAchievementRow* GetSelectedAchievement();
	void RefreshCategoryList();
	void RefreshAchievementList();
	void RefreshObjectiveList();
	void RefreshRewardList();
	void SelectCategoryListRow();
	void SelectAchievementListRow();
	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CListWnd* CategoryList = nullptr;
	CListWnd* AchievementList = nullptr;
	CXWnd* DetailTitleLabel = nullptr;
	CXWnd* DetailDescriptionLabel = nullptr;
	CListWnd* ObjectiveList = nullptr;
	CListWnd* RewardList = nullptr;
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* CheckButton = nullptr;
	int SelectedCategoryID = 0;
	int SelectedAchievementID = 0;
};

static NativeAchievementWnd* gNativeAchievementWnd = nullptr;
static std::vector<NativeAchievementCategoryRow> gNativeAchievementCategories;
static std::vector<NativeAchievementRow> gNativeAchievementRows;
static std::vector<NativeAchievementObjectiveRow> gNativeAchievementObjectives;
static std::vector<NativeAchievementRewardRow> gNativeAchievementRewards;
static int gNativeAchievementSelectedCategory = 0;
static int gNativeAchievementSelectedAchievement = 0;
static int gNativeAchievementCompleted = 0;
static int gNativeAchievementTotal = 0;
static int gNativeAchievementPoints = 0;
static int gNativeAchievementCategoryCount = 0;
static bool gNativeAchievementLoading = false;
static bool gNativeAchievementCategoriesDirty = true;
static bool gNativeAchievementRowsDirty = true;
static bool gNativeAchievementObjectivesDirty = true;
static bool gNativeAchievementRewardsDirty = true;
static bool gNativeAchievementsHooksInstalled = false;
static bool gNativeAchievementsUiResetHookInstalled = false;
static std::string gNativeAchievementDetailTitle = "Select an achievement";
static std::string gNativeAchievementDetailDescription;

void NativeAchievementWnd::RefreshCategoryList()
{
	if (!CategoryList) {
		return;
	}

	CategoryList->DeleteAll();
	if (gNativeAchievementCategories.empty()) {
		CXStr dash("-");
		const int row = CategoryList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No categories loaded.");
		CategoryList->SetItemText(row, 1, &empty);
		CategoryList->SetItemText(row, 2, &dash);
		gNativeAchievementCategoriesDirty = false;
		return;
	}

	int selected_row = -1;
	for (const NativeAchievementCategoryRow& category : gNativeAchievementCategories) {
		std::string display = category.parent_id ? "  " + category.name : category.name;
		CXStr name(display.c_str());
		const COLORREF row_color = category.total <= 0 ? 0xFFB0B0B0 :
			(category.completed >= category.total ? 0xFF66FF66 : 0xFFFFFFFF);
		const int row = CategoryList->AddString(name, row_color, (uint32_t)category.id, nullptr, nullptr);

		char progress_text[32];
		sprintf_s(progress_text, "%d/%d", category.completed, category.total);
		char points_text[32];
		sprintf_s(points_text, "%d", category.points);
		CXStr progress(progress_text);
		CXStr points(points_text);
		CategoryList->SetItemText(row, 1, &progress);
		CategoryList->SetItemText(row, 2, &points);

		if (category.id == SelectedCategoryID) {
			selected_row = row;
		}
	}

	if (selected_row >= 0) {
		CategoryList->SetCurSel(selected_row);
	}

	gNativeAchievementCategoriesDirty = false;
}

void NativeAchievementWnd::RefreshAchievementList()
{
	if (!AchievementList) {
		return;
	}

	AchievementList->DeleteAll();
	if (gNativeAchievementRows.empty()) {
		CXStr dash("-");
		const int row = AchievementList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No achievements in this category.");
		AchievementList->SetItemText(row, 1, &empty);
		AchievementList->SetItemText(row, 2, &dash);
		gNativeAchievementRowsDirty = false;
		return;
	}

	int selected_row = -1;
	for (const NativeAchievementRow& achievement : gNativeAchievementRows) {
		CXStr state(achievement.completed ? "Done" : "Open");
		const COLORREF row_color = achievement.completed ? 0xFF66FF66 : 0xFFFFFFFF;
		const int row = AchievementList->AddString(state, row_color, (uint32_t)achievement.id, nullptr, nullptr);

		char points_text[32];
		sprintf_s(points_text, "%d", achievement.points);
		CXStr name(achievement.name.c_str());
		CXStr points(points_text);
		CXStr description(achievement.description.c_str());
		AchievementList->SetItemText(row, 1, &name);
		AchievementList->SetItemText(row, 2, &points);
		AchievementList->SetItemText(row, 3, &description);

		if (achievement.id == SelectedAchievementID) {
			selected_row = row;
		}
	}

	if (selected_row >= 0) {
		AchievementList->SetCurSel(selected_row);
	}

	gNativeAchievementRowsDirty = false;
}

void NativeAchievementWnd::RefreshObjectiveList()
{
	if (!ObjectiveList) {
		return;
	}

	ObjectiveList->DeleteAll();
	if (gNativeAchievementObjectives.empty()) {
		CXStr dash("-");
		const int row = ObjectiveList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No objectives loaded.");
		ObjectiveList->SetItemText(row, 1, &empty);
		ObjectiveList->SetItemText(row, 2, &dash);
		gNativeAchievementObjectivesDirty = false;
		return;
	}

	for (const NativeAchievementObjectiveRow& objective : gNativeAchievementObjectives) {
		CXStr state(objective.completed ? "Done" : "Open");
		const int row = ObjectiveList->AddString(state, objective.completed ? 0xFF66FF66 : 0xFFFFFFFF, (uint32_t)objective.id, nullptr, nullptr);

		char progress_text[32];
		sprintf_s(progress_text, "%d/%d", objective.current, objective.required);
		CXStr name(objective.name.c_str());
		CXStr progress(progress_text);
		ObjectiveList->SetItemText(row, 1, &name);
		ObjectiveList->SetItemText(row, 2, &progress);
	}

	gNativeAchievementObjectivesDirty = false;
}

void NativeAchievementWnd::RefreshRewardList()
{
	if (!RewardList) {
		return;
	}

	RewardList->DeleteAll();
	if (gNativeAchievementRewards.empty()) {
		CXStr dash("-");
		const int row = RewardList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No rewards listed.");
		RewardList->SetItemText(row, 1, &empty);
		gNativeAchievementRewardsDirty = false;
		return;
	}

	for (const NativeAchievementRewardRow& reward : gNativeAchievementRewards) {
		CXStr state(reward.auto_claim ? "Auto" : "Claim");
		const int row = RewardList->AddString(state, reward.auto_claim ? 0xFF66FF66 : 0xFFFFFF80, (uint32_t)reward.definition_id, nullptr, nullptr);

		std::string reward_text = reward.name.empty() ? reward.type : reward.name;
		if (!reward.tier.empty()) {
			reward_text += " [";
			reward_text += reward.tier;
			reward_text += "]";
		}

		CXStr name(reward_text.c_str());
		RewardList->SetItemText(row, 1, &name);
	}

	gNativeAchievementRewardsDirty = false;
}

void NativeAchievementWnd::SelectCategoryListRow()
{
	if (!CategoryList || SelectedCategoryID <= 0) {
		return;
	}

	for (size_t i = 0; i < gNativeAchievementCategories.size(); ++i) {
		if (gNativeAchievementCategories[i].id == SelectedCategoryID) {
			CategoryList->SetCurSel((int)i);
			return;
		}
	}
}

void NativeAchievementWnd::SelectAchievementListRow()
{
	if (!AchievementList || SelectedAchievementID <= 0) {
		return;
	}

	for (size_t i = 0; i < gNativeAchievementRows.size(); ++i) {
		if (gNativeAchievementRows[i].id == SelectedAchievementID) {
			AchievementList->SetCurSel((int)i);
			return;
		}
	}
}

void NativeAchievementWnd::RefreshRows()
{
	SelectedCategoryID = gNativeAchievementSelectedCategory;
	SelectedAchievementID = gNativeAchievementSelectedAchievement;

	char summary[160];
	sprintf_s(
		summary,
		"Achievements %d/%d complete  Points %d  Categories %d",
		gNativeAchievementCompleted,
		gNativeAchievementTotal,
		gNativeAchievementPoints,
		gNativeAchievementCategoryCount
	);
	SetLabel(SummaryLabel, summary);
	SetLabel(DetailTitleLabel, gNativeAchievementDetailTitle.c_str());
	SetLabel(DetailDescriptionLabel, gNativeAchievementDetailDescription.c_str());

	if (gNativeAchievementCategoriesDirty) {
		RefreshCategoryList();
	}
	else {
		SelectCategoryListRow();
	}

	if (gNativeAchievementRowsDirty) {
		RefreshAchievementList();
	}
	else {
		SelectAchievementListRow();
	}

	if (gNativeAchievementObjectivesDirty) {
		RefreshObjectiveList();
	}

	if (gNativeAchievementRewardsDirty) {
		RefreshRewardList();
	}
}

NativeAchievementCategoryRow* NativeAchievementWnd::GetSelectedCategory()
{
	if (!CategoryList) {
		return nullptr;
	}

	const int selected = CategoryList->GetCurSel();
	if (selected < 0) {
		return nullptr;
	}

	const int category_id = (int)CategoryList->GetItemData(selected);
	if (category_id <= 0) {
		return nullptr;
	}

	for (NativeAchievementCategoryRow& category : gNativeAchievementCategories) {
		if (category.id == category_id) {
			return &category;
		}
	}

	return nullptr;
}

NativeAchievementRow* NativeAchievementWnd::GetSelectedAchievement()
{
	if (!AchievementList) {
		return nullptr;
	}

	const int selected = AchievementList->GetCurSel();
	if (selected < 0) {
		return nullptr;
	}

	const int achievement_id = (int)AchievementList->GetItemData(selected);
	if (achievement_id <= 0) {
		return nullptr;
	}

	for (NativeAchievementRow& achievement : gNativeAchievementRows) {
		if (achievement.id == achievement_id) {
			return &achievement;
		}
	}

	return nullptr;
}

static void NativeAchievementEnsureWindow(bool show)
{
	// AoTv4: our dll historically only drew GDI overlays and never used the native SIDL UI managers,
	// so wire ppSidlMgr/ppWndMgr straight from the client globals if they weren't set up
	// (pinstCSidlManager_x=0x15D3D08, pinstCXWndManager_x=0x15D3D00; rebased via baseAddress).
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	NativeAchievementsTrace("EnsureWindow: pSidlMgr=%p pWndMgr=%p localPlayer=%p wnd=%p",
		(void *)pSidlMgr, (void *)pWndMgr, (void *)pLocalPlayer, (void *)gNativeAchievementWnd);

	if (!pSidlMgr || !pWndMgr) {
		NativeAchievementsTrace("EnsureWindow: UI managers still null -- aborting (UI not ready?)");
		return;
	}

	if (!gNativeAchievementWnd) {
		NativeAchievementsTrace("creating achievement window");
		gNativeAchievementWnd = new NativeAchievementWnd();
		gNativeAchievementWnd->RefreshRows();
		NativeAchievementsTrace("achievement window created (xwnd=%p)",
			gNativeAchievementWnd ? (void *)gNativeAchievementWnd->pXWnd() : nullptr);
	}

	if (show && gNativeAchievementWnd) {
		gNativeAchievementWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeAchievementsSendCommand(const char* command)
{
	if (!command || !command[0] || !pEverQuest || !pLocalPlayer) {
		return;
	}

	NativeAchievementsTrace("send command: %s", command);
	char buffer[256];
	strcpy_s(buffer, command);
	pEverQuest->InterpretCmd((EQPlayer*)pLocalPlayer, buffer);
}

bool NativeAchievementParseTransport(const char* message)
{
	if (!message || !message[0] || !NativeStartsWith(message, "ACH|")) {
		return false;
	}

	if (NativeStartsWith(message, "ACH|window|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementCategories.clear();
		gNativeAchievementRows.clear();
		gNativeAchievementObjectives.clear();
		gNativeAchievementRewards.clear();
		gNativeAchievementSelectedCategory = 0;
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementCategoriesDirty = true;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementRewardsDirty = true;
		gNativeAchievementDetailTitle = "Select an achievement";
		gNativeAchievementDetailDescription.clear();
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievements...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|achievements|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementRows.clear();
		gNativeAchievementObjectives.clear();
		gNativeAchievementRewards.clear();
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementRewardsDirty = true;
		gNativeAchievementDetailTitle = "Select an achievement";
		gNativeAchievementDetailDescription.clear();
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievement category...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|objectives|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementObjectives.clear();
		gNativeAchievementObjectivesDirty = true;
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievement detail...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|rewards|clear")) {
		gNativeAchievementRewards.clear();
		gNativeAchievementRewardsDirty = true;
		return true;
	}

	if (NativeStartsWith(message, "ACH|status|")) {
		const std::string payload(message + strlen("ACH|status|"));
		gNativeAchievementCompleted = NativeToInt(NativeGetPairValue(payload, "completed"));
		gNativeAchievementTotal = NativeToInt(NativeGetPairValue(payload, "total"));
		gNativeAchievementPoints = NativeToInt(NativeGetPairValue(payload, "points"));
		gNativeAchievementCategoryCount = NativeToInt(NativeGetPairValue(payload, "categories"));
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|category|")) {
		const std::string payload(message + strlen("ACH|category|"));
		NativeAchievementCategoryRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.parent_id = NativeToInt(NativeGetPairValue(payload, "parent"));
		row.name = NativeGetPairValue(payload, "name");
		row.completed = NativeToInt(NativeGetPairValue(payload, "completed"));
		row.total = NativeToInt(NativeGetPairValue(payload, "total"));
		row.points = NativeToInt(NativeGetPairValue(payload, "points"));
		if (row.id > 0) {
			// AoTv4: upsert by id -- during a full load the list was cleared so everything is new
			// (push), but a live completion update re-sends categories WITHOUT a clear, so update the
			// existing row's counts in place (keeps the player's current view/selection intact).
			bool found = false;
			for (auto &existing : gNativeAchievementCategories) {
				if (existing.id == row.id) { existing = row; found = true; break; }
			}
			if (!found) gNativeAchievementCategories.push_back(row);
			gNativeAchievementCategoriesDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	// AoTv4: targeted "flip one achievement's Done state" for the live completion update. Updates the row
	// in place if it's in the currently-viewed category; silently ignored otherwise (never appends, so it
	// can't pollute another category's list). Separate from ACH|achievement| (the full-load append).
	if (NativeStartsWith(message, "ACH|achievement_state|")) {
		const std::string payload(message + strlen("ACH|achievement_state|"));
		const int id = NativeToInt(NativeGetPairValue(payload, "id"));
		const bool completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		for (auto &existing : gNativeAchievementRows) {
			if (existing.id == id) { existing.completed = completed; gNativeAchievementRowsDirty = true; break; }
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|selection|")) {
		const std::string payload(message + strlen("ACH|selection|"));
		gNativeAchievementSelectedCategory = NativeToInt(NativeGetPairValue(payload, "category"));
		gNativeAchievementSelectedAchievement = NativeToInt(NativeGetPairValue(payload, "achievement"));
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|achievement|")) {
		const std::string payload(message + strlen("ACH|achievement|"));
		NativeAchievementRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.category_id = NativeToInt(NativeGetPairValue(payload, "category"));
		row.completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		row.points = NativeToInt(NativeGetPairValue(payload, "points"));
		row.name = NativeGetPairValue(payload, "name");
		row.description = NativeGetPairValue(payload, "description");
		if (row.id > 0) {
			gNativeAchievementRows.push_back(row);
			gNativeAchievementRowsDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|detail|")) {
		const std::string payload(message + strlen("ACH|detail|"));
		gNativeAchievementSelectedAchievement = NativeToInt(NativeGetPairValue(payload, "id"), gNativeAchievementSelectedAchievement);
		const bool completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		const int points = NativeToInt(NativeGetPairValue(payload, "points"));
		const std::string name = NativeGetPairValue(payload, "name");
		const std::string description = NativeGetPairValue(payload, "description");
		char title[160];
		sprintf_s(title, "%s [%s, %d pts]", name.empty() ? "Achievement" : name.c_str(), completed ? "Done" : "Open", points);
		gNativeAchievementDetailTitle = title;
		gNativeAchievementDetailDescription = description;
		gNativeAchievementObjectives.clear();
		gNativeAchievementObjectivesDirty = true;
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|objective|")) {
		const std::string payload(message + strlen("ACH|objective|"));
		NativeAchievementObjectiveRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		row.current = NativeToInt(NativeGetPairValue(payload, "current"));
		row.required = NativeToInt(NativeGetPairValue(payload, "required"));
		row.name = NativeGetPairValue(payload, "name");
		if (row.id > 0) {
			gNativeAchievementObjectives.push_back(row);
			gNativeAchievementObjectivesDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|reward|")) {
		const std::string payload(message + strlen("ACH|reward|"));
		NativeAchievementRewardRow row;
		row.definition_id = static_cast<uint64_t>(_strtoui64(NativeGetPairValue(payload, "definition").c_str(), nullptr, 10));
		row.type = NativeGetPairValue(payload, "type");
		row.reward_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.amount = NativeToInt(NativeGetPairValue(payload, "amount"));
		row.auto_claim = NativeToBool(NativeGetPairValue(payload, "auto"));
		row.tier = NativeGetPairValue(payload, "tier");
		row.name = NativeGetPairValue(payload, "name");
		if (row.definition_id > 0 || !row.name.empty()) {
			gNativeAchievementRewards.push_back(row);
			gNativeAchievementRewardsDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|window|show")) {
		gNativeAchievementLoading = false;
		NativeAchievementEnsureWindow(true);
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->RefreshRows();
			gNativeAchievementWnd->SetStatus("Achievements loaded.");
		}
		return true;
	}

	return true;
}

bool NativeAchievementRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/achievement", &arguments) &&
		!NativeCommandMatch(line, "/achievements", &arguments) &&
		!NativeCommandMatch(line, "/achivement", &arguments) &&
		!NativeCommandMatch(line, "/achivements", &arguments) &&
		!NativeCommandMatch(line, "/ach", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #ach native");
		return true;
	}

	if (
		NativeCommandMatch(arguments, "open", nullptr) ||
		NativeCommandMatch(arguments, "window", nullptr) ||
		NativeCommandMatch(arguments, "ui", nullptr) ||
		NativeCommandMatch(arguments, "panel", nullptr)
	) {
		strcpy_s(output, output_size, "/say #ach native");
		return true;
	}

	if (
		NativeCommandMatch(arguments, "refresh", nullptr) ||
		NativeCommandMatch(arguments, "status", nullptr) ||
		NativeCommandMatch(arguments, "show", nullptr) ||
		NativeCommandMatch(arguments, "snapshot", nullptr)
	) {
		sprintf_s(output, output_size, "/say #ach native refresh %d %d", gNativeAchievementSelectedCategory, gNativeAchievementSelectedAchievement);
		return true;
	}

	if (NativeCommandMatch(arguments, "check", nullptr)) {
		sprintf_s(output, output_size, "/say #ach native check %d %d", gNativeAchievementSelectedCategory, gNativeAchievementSelectedAchievement);
		return true;
	}

	sprintf_s(output, output_size, "/say #ach %s", arguments);
	return true;
}

bool NativeAchievementHandleLocalCommand(const char* line)
{
	if (!line) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/achievement", &arguments) &&
		!NativeCommandMatch(line, "/achievements", &arguments) &&
		!NativeCommandMatch(line, "/achivement", &arguments) &&
		!NativeCommandMatch(line, "/achivements", &arguments) &&
		!NativeCommandMatch(line, "/ach", &arguments)
	) {
		return false;
	}

	if (arguments && NativeCommandMatch(arguments, "close", nullptr)) {
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->pXWnd()->Show(0, 1);
		}
		return true;
	}

	return false;
}

// The chat + slash-command detours the upstream reference installed here have been
// REMOVED. Our dll already detours CEverQuest::dsp_chat (core_spellwindow.cpp) and
// CEverQuest::InterpretCmd (core_bazaar.h); those bodies now call the plain
// NativeAchievementParseTransport / HandleLocalCommand / RewriteCommand functions.
// Only the UI-reset detours (no conflict) are installed locally, below.

// AoTv4 native shop window (core_spellwindow.cpp) shares this UI-reset -- it must be torn down on
// the same CleanGameUI/ReloadUI, and this module owns those hooks.
extern void ShopWndOnUiReset();

class NativeAchievementsUiResetHook
{
public:
	VOID CleanUI_Trampoline(VOID);
	VOID CleanUI_Detour(VOID)
	{
		if (gNativeAchievementWnd) {
			delete gNativeAchievementWnd;
			gNativeAchievementWnd = nullptr;
		}
		ShopWndOnUiReset();
		AdvLootOnUiReset();
		SpellChoiceOnUiReset();
		LostWindowOnUiReset();
		AutoSkillOnUiReset();
		DungeonOnUiReset();
		DifficultyOnUiReset();
		FctWindowOnUiReset();
		MeterWindowOnUiReset();
		FellowshipOnUiReset();
		TravelOnUiReset();
		FloatingTextOnUiReset();
		AllacloneOnUiReset();
		AoTMenuOnUiReset();
		CleanUI_Trampoline();
	}

	VOID ReloadUI_Trampoline(BOOL use_ini);
	VOID ReloadUI_Detour(BOOL use_ini)
	{
		if (gNativeAchievementWnd) {
			delete gNativeAchievementWnd;
			gNativeAchievementWnd = nullptr;
		}
		ShopWndOnUiReset();
		AdvLootOnUiReset();
		SpellChoiceOnUiReset();
		LostWindowOnUiReset();
		AutoSkillOnUiReset();
		DungeonOnUiReset();
		DifficultyOnUiReset();
		FctWindowOnUiReset();
		MeterWindowOnUiReset();
		FellowshipOnUiReset();
		TravelOnUiReset();
		FloatingTextOnUiReset();
		AllacloneOnUiReset();
		AoTMenuOnUiReset();
		ReloadUI_Trampoline(use_ini);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID NativeAchievementsUiResetHook::CleanUI_Trampoline(VOID));
DETOUR_TRAMPOLINE_EMPTY(VOID NativeAchievementsUiResetHook::ReloadUI_Trampoline(BOOL));

static void NativeAchievementsInstallUiResetHook()
{
	if (gNativeAchievementsUiResetHookInstalled) {
		return;
	}

	NativeAchievementsTrace("installing achievement UI reset hook");
	EzDetour(CDisplay__CleanGameUI, &NativeAchievementsUiResetHook::CleanUI_Detour, &NativeAchievementsUiResetHook::CleanUI_Trampoline);
	EzDetour(CDisplay__ReloadUI, &NativeAchievementsUiResetHook::ReloadUI_Detour, &NativeAchievementsUiResetHook::ReloadUI_Trampoline);
	gNativeAchievementsUiResetHookInstalled = true;
}

void InitAchievementsNative()
{
	if (gNativeAchievementsHooksInstalled) {
		return;
	}

	gNativeAchievementsHooksInstalled = true;
	// Chat + command hooks are NOT installed here -- our shared detours route to us
	// (see core_spellwindow.cpp dsp_chat + core_bazaar.h InterpretCmd_TA_Detour).
	NativeAchievementsInstallUiResetHook();
}

static void ShutdownAchievementsNative()
{
	NativeAchievementsTrace("shutdown");

	if (gNativeAchievementWnd) {
		delete gNativeAchievementWnd;
		gNativeAchievementWnd = nullptr;
	}

	if (gNativeAchievementsUiResetHookInstalled) {
		RemoveDetour(CDisplay__CleanGameUI);
		RemoveDetour(CDisplay__ReloadUI);
		gNativeAchievementsUiResetHookInstalled = false;
	}

	gNativeAchievementsHooksInstalled = false;
	gNativeAchievementCategories.clear();
	gNativeAchievementRows.clear();
	gNativeAchievementObjectives.clear();
	gNativeAchievementRewards.clear();
	gNativeAchievementSelectedCategory = 0;
	gNativeAchievementSelectedAchievement = 0;
	gNativeAchievementLoading = false;
	gNativeAchievementCategoriesDirty = true;
	gNativeAchievementRowsDirty = true;
	gNativeAchievementObjectivesDirty = true;
	gNativeAchievementRewardsDirty = true;
	gNativeAchievementDetailTitle = "Select an achievement";
	gNativeAchievementDetailDescription.clear();
}
