// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "pch.h"
#include "StaticSceneEntitySoundSource.h"
#include "AStaticSceneEntity.h"
#include "ConstStaticsStateAccessor.h"
// --------------------------------------------------------------------------
GENRESULT COMAPI StaticSceneEntitySoundSource::get_position(Vector *position)
{
	ASSERT(mStaticSE);

	const ROS::Location	location = mStaticSE->GetConstStaticsStateAccessor()->GetLocation();

	*position = location.GetVector();

	return GR_OK;
}
// --------------------------------------------------------------------------
