#include "Entity.h"

#include "EngineTrace.h"
#include "Localize.h"
#include "PlayerResource.h"

bool Entity::isOtherEnemy(Entity *other) noexcept
{
	return memory->isOtherEnemy(this, other);
}

bool Entity::hasC4() noexcept
{
	if (!*memory->playerResource)
		return false;

	return this->isPlayer() && this->index() == (*memory->playerResource)->bombOwner();
}

bool Entity::isVip() noexcept
{
	if (!*memory->playerResource)
		return false;

	return this->isPlayer() && this->index() == (*memory->playerResource)->vip();
}

void Entity::getPlayerName(char (&out)[128]) noexcept
{
	if (!*memory->playerResource)
	{
		strcpy(out, "unknown");
		return;
	}

	wchar_t wide[128];
	memory->getDecoratedPlayerName(*memory->playerResource, index(), wide, sizeof(wide), 4);

	auto end = std::remove(wide, wide + wcslen(wide), L'\n');
	*end = L'\0';
	end = std::unique(wide, end, [](wchar_t a, wchar_t b) { return a == L' ' && a == b; });
	*end = L'\0';

	interfaces->localize->convertUnicodeToAnsi(wide, out, 128);
}

bool Entity::canSee(Entity *other, const Vector &pos) noexcept
{
	const auto eyePos = getEyePosition();
	if (memory->lineGoesThroughSmoke(eyePos, pos, 1))
		return false;

	Trace trace;
	interfaces->engineTrace->traceRay({eyePos, pos}, MASK_SHOT & ~CONTENTS_WINDOW, this, trace);
	return trace.entity == other || trace.fraction > 0.97f;
}

bool Entity::visibleTo(Entity *other) noexcept
{
	assert(isAlive());

	Entity *targetEntity = other;
	if (!other->isAlive() && other->getObserverMode() == ObsMode::InEye && other->getObserverTarget())
		targetEntity = other->getObserverTarget();

	if (targetEntity->canSee(this, getAbsOrigin() + Vector{0.0f, 0.0f, 5.0f}))
		return true;

	if (targetEntity->canSee(this, getEyePosition() + Vector{0.0f, 0.0f, 5.0f}))
		return true;

	const auto model = getModel();
	if (!model)
		return false;

	const auto studioModel = interfaces->modelInfo->getStudioModel(model);
	if (!studioModel)
		return false;

	const auto set = studioModel->getHitboxSet(hitboxSet());
	if (!set)
		return false;

	Matrix3x4 boneMatrices[MAX_STUDIO_BONES];
	if (!setupBones(boneMatrices, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
		return false;

	for (const int boxNum : {Hitbox_Belly, Hitbox_LeftForearm, Hitbox_RightForearm})
	{
		const auto hitbox = set->getHitbox(boxNum);
		if (hitbox && targetEntity->canSee(this, boneMatrices[hitbox->bone].origin()))
			return true;
	}

	return false;
}
