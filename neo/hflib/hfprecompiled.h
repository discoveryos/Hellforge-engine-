#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#include "sys/hfsys_defines.h"
#include "sys/hfsys_builddefines.h"
#include "sys/hfsys_includes.h"
#include "sys/hfsys_assert.h"
#include "sys/hfsys_types.h"
#include "sys/hfsys_intrinsics.h"
#include "sys/hfsys_threading.h"

//-----------------------------------------------------

// base configuration, types and inline helpers (must come first)
#include "hfConfig.h"
#include "hfTypes.h"
#include "hfDefs.h"
#include "hfInline.h"

#define HF_TIME_T int64 // Signed because -1 means "File not found" and we don't want that to compare > than any other time

// non-portable system services
#include "../sys/hfsys_public.h"

// id lib
#include "../hflib/hfLib.h"

#include "sys/hfsys_filesystem.h"

// framework
#include "../framework/hfBuildVersion.h"
#include "../framework/hfLicensee.h"
#include "../framework/hfCmdSystem.h"
#include "../framework/hfCVarSystem.h"
#include "../framework/hfCommon.h"
#include "../framework/hfFile.h"
#include "../framework/hfFile_Manifest.h"
#include "../framework/hfFile_SaveGame.h"
#include "../framework/hfFile_Resource.h"
#include "../framework/hfFileSystem.h"
#include "../framework/hfUsercmdGen.h"
#include "../framework/hfSerializer.h"
#include "../framework/hfPlayerProfile.h"

// decls
#include "../framework/hfTokenParser.h"
#include "../framework/hfDeclManager.h"
#include "../framework/hfDeclTable.h"
#include "../framework/hfDeclSkin.h"
#include "../framework/hfDeclEntityDef.h"
#include "../framework/hfDeclFX.h"
#include "../framework/hfDeclParticle.h"
#include "../framework/hfDeclAF.h"
#include "../framework/hfDeclPDA.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "../renderer/OpenGL/hfqgl.h"
#include "../renderer/hfCinematic.h"
#include "../renderer/hfMaterial.h"
#include "../renderer/hfBufferObject.h"
#include "../renderer/hfVertexCache.h"
#include "../renderer/hfModel.h"
#include "../renderer/hfModelManager.h"
#include "../renderer/hfRenderSystem.h"
#include "../renderer/hfRenderWorld.h"

// sound engine
#include "../sound/hfsound.h"

// user interfaces
#include "../ui/hfListGUI.h"
#include "../ui/hfUserInterface.h"

#include "../swf/hfSWF.h"

// collision detection system
#include "../cm/hfCollisionModel.h"

// AAS files and manager
#include "../aas/hfAASFile.h"
#include "../aas/hfAASFileManager.h"

// game
#include "../framework/hfGameModule.h"

// Session / Network
#include "../sys/hfLightweightCompression.h"
#include "../sys/hfSnapshot.h"
#include "../sys/hfPacketProcessor.h"
#include "../sys/hfSnapshotProcessor.h"

#include "../sys/hfsys_savegame.h"
#include "../sys/hfsys_session_savegames.h"
#include "../sys/hfsys_profile.h"
#include "../sys/hfsys_localuser.h"
#include "../sys/hfsys_signin.h"
#include "../sys/hfsys_stats_misc.h"
#include "../sys/hfsys_stats.h"
#include "../sys/hfsys_session.h"
#include "../sys/hfsys_achievements.h"

//-----------------------------------------------------

#ifndef _D3SDK

#ifdef GAME_DLL

// Game DLLs include their own game session implementation
#include "../framework/hfGameModule.h"

#else

#include "../framework/hfDemoChecksum.h"

// framework
#include "../framework/hfCompressor.h"
#include "../framework/hfEventLoop.h"
#include "../framework/hfKeyInput.h"
#include "../framework/hfEditField.h"
#include "../framework/hfDebugGraph.h"
#include "../framework/hfConsole.h"
#include "../framework/hfDemoFile.h"
#include "../framework/hfCommon_dialog.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#undef min
#undef max
#include <algorithm>	// for min / max / swap

#endif /* !__PRECOMPILED_H__ */

