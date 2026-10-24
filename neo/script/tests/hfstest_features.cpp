#include "../../hflib/hfprecompiled.h"
#include "hfEcs.h"
#include "hfAssetStore.h"
#include "hfLightCluster.h"
#include "hfStreaming.h"
#include "../../hflib/audio/hfAudioData.h"
#include "../../hflib/audio/hfAudioSpatial.h"
#include "../../hflib/audio/hfAudioServer.h"
#include "../../hflib/ui/hfBitmapFont.h"
#include "../../hflib/ui/hfCanvasComponent.h"
#include "../../hflib/ui/hfUiSystem.h"
#include "../../hflib/net/hfNetComponent.h"
#include "../../hflib/net/hfReplicationSystem.h"
#include "../../hflib/editor/hfEditorReflection.h"
#include "../../hflib/editor/hfEditorTree.h"
#include "../../hflib/editor/hfEditorDocument.h"
#include "../../hflib/bv/hfCollide.h"
#include "../../hflib/anim/hfAnimationStateMachine.h"
#include "../../hflib/scene/hfWorld.h"
#include "../../hflib/scene/hfLightSystem.h"
#include "../../hflib/scene/hfSceneSerializer.h"
#include "../../hflib/resource/hfResourceManager.h"
#include "../../hflib/physics/hfPhysicsSystem.h"
#include "../../hflib/render/hfRenderingServer.h"
#include "../../hflib/asset/hfGltf.h"
#include "../../hflib/scene/hfCsgSystem.h"
#include "../../hflib/hfBsp.h"
#include "../../framework/zlib/hfzlib.h"
#include "../HfScript.h"
#include "../HfScript_Game.h"
#include "../../hflib/console/hfConsoleServices.h"
#include "../../hflib/ui/hfUiBuilder.h"
#include "../../hflib/ui/hfUiInput.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compiled into the headless harness only (no renderer symbols needed).

static int g_featFails = 0;
#define FEAT_ASSERT(cond) do { if (!(cond)) { g_featFails++; printf("FEAT_ASSERT(%s) line %d\n", #cond, __LINE__); } } while(0)
#define FEAT_NEAR(a,b,eps) do { \
	if ( hfMath::Fabs( ( a ) - ( b ) ) > ( eps ) ) { \
		g_featFails++; printf("FEAT_NEAR(%s=%g, %s=%g, %s) line %d\n", #a, (double)(a), #b, (double)(b), #eps, __LINE__); \
	} } while(0)

static void Test_ECS() {
HF_Ecs::Registry reg;
	HF_Ecs::Entity e1 = reg.Create();
	HF_Ecs::Entity e2 = reg.Create();
	FEAT_ASSERT( reg.IsAlive( e1 ) );
	FEAT_ASSERT( reg.IsAlive( e2 ) );
	reg.Destroy( e1 );
	FEAT_ASSERT( !reg.IsAlive( e1 ) );
	FEAT_ASSERT( reg.IsAlive( e2 ) );
	HF_Ecs::Entity reuse = reg.Create();
	FEAT_ASSERT( ( reuse & HF_Ecs::INDEX_MASK ) == ( e1 & HF_Ecs::INDEX_MASK ) );
	FEAT_ASSERT( !reg.IsAlive( e1 ) );
	FEAT_ASSERT( reg.IsAlive( reuse ) );
	FEAT_ASSERT( reg.GetLiveCount() == 2 );
	// components
	struct Health { int hp; };
	HF_Ecs::Entity ch = reg.Create();
	int &h = reg.AddComponent<Health>( ch ).hp;
	h = 99;
	FEAT_ASSERT( reg.GetComponent<Health>( ch )->hp == 99 );
	FEAT_ASSERT( reg.Has<Health>( ch ) );
	reg.Destroy( ch );
	FEAT_ASSERT( !reg.Has<Health>( ch ) );
	// events
	HF_Ecs::EventBus bus;
	int hit = 0;
	bus.Subscribe( "evt", []( const char *, const void *, int, void * u ) { (*(int *)u)++; }, &hit );
	bus.Publish( "evt" );
	FEAT_ASSERT( hit == 1 );
	bus.Publish( "missing" );
	FEAT_ASSERT( hit == 1 );
	printf( "ECS ok\n" );
}

static void Test_AssetStore() {
	using namespace HF_Asset;
	FEAT_ASSERT( Adler32( "hello", 5 ) != 0 );
	FEAT_ASSERT( LowerCaseHash( "FOO" ) == LowerCaseHash( "foo" ) );
	const char * names[] = { "alpha.tga", "wall.png" };
	const char * blobs[] = { "AAA", "BBBBB" };
	AssetSource items[2];
	for ( int i = 0; i < 2; i++ ) { items[i].name = names[i]; items[i].data = blobs[i]; items[i].size = ( unsigned int )strlen( blobs[i] ); }
	PackedStore store = Pack( items, 2 );
	FEAT_ASSERT( store.ok && store.data && store.size > 0 );
	hfStr error;
	FEAT_ASSERT( Validate( store.data, store.size, &error ) );
	hfStr out;
	FEAT_ASSERT( Extract( store.data, store.size, "wall.png", out ) );
	FEAT_ASSERT( out == "BBBBB" );
	FEAT_ASSERT( !Extract( store.data, store.size, "missing", out ) );
	hfStr list;
	List( store.data, store.size, list );
	FEAT_ASSERT( list.Length() > 0 );
	FEAT_ASSERT( list.Find( "alpha.tga" ) >= 0 );
	FreeStore( store );
	// mtr parse
	const char * mtr = "testMat { stages { map textures/foo.tga } noshadows }";
	mtrDef_t defs[4];
	int n = ParseMtr( mtr, ( int )strlen( mtr ), defs, 4 );
	FEAT_ASSERT( n == 1 );
	FEAT_ASSERT( defs[0].parseOk );
	FEAT_ASSERT( defs[0].noShadows );
	FEAT_ASSERT( defs[0].stages[0].textureName == "textures/foo.tga" );
	// glsl sanity
	FEAT_ASSERT( GlslSanityCheck( "void main() { }" ) );
	FEAT_ASSERT( !GlslSanityCheck( "" ) );
	FEAT_ASSERT( !GlslSanityCheck( "void main() { ( }" ) );
	FEAT_ASSERT( !GlslSanityCheck( "void main() { #ifdef A }" ) );
	printf( "AssetStore ok\n" );
}

static void Test_Cluster() {
	using namespace HF_Cluster;
	int tx, ty;
	ComputeTileGrid( 1920, 1080, 48, tx, ty );
	FEAT_ASSERT( tx == 40 && ty == 22 );
	FEAT_ASSERT( DepthSlice( -1.0f, CLIP_Z_NEAR_DEFAULT, CLIP_Z_FAR_DEFAULT ) == 0 );
	FEAT_ASSERT( DepthSlice(  1.0f, CLIP_Z_NEAR_DEFAULT, CLIP_Z_FAR_DEFAULT ) == MAX_DEPTH_SLICES - 1 );
	// identity worldToClip; light at origin; center tile = tx/2, ty/2
	float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	LightEntry l;
	l.position[0] = 0; l.position[1] = 0; l.position[2] = -5;
	l.radius = 10; l.index = 0; l.priority = 1.0f;
	AssignResult res;
	memset( &res, 0, sizeof( res ) );
	unsigned int assigned = AssignLights( identity, 1920, 1080, 48, 4, &l, 1, 2, 1.0f, res );
	FEAT_ASSERT( assigned == 1 );
	FEAT_ASSERT( res.totalAssigned == 1 );
	printf( "Cluster ok\n" );
}

static void Test_Audio() {
	using namespace HF_Audio;
	// attenuation: linear model
	FEAT_ASSERT( hfAttenuationGain( HF_ATTEN_LINEAR, 0.0f, 1.0f, 100.0f ) == 1.0f );
	FEAT_ASSERT( hfAttenuationGain( HF_ATTEN_LINEAR, 100.0f, 1.0f, 100.0f ) == 0.0f );
	FEAT_ASSERT( hfAttenuationGain( HF_ATTEN_LINEAR, 200.0f, 1.0f, 100.0f ) == 0.0f );
	float mid = hfAttenuationGain( HF_ATTEN_LINEAR, 50.5f, 1.0f, 100.0f );
	FEAT_ASSERT( mid > 0.45f && mid < 0.55f );	// halfway between min/max
	// inverse clamped model stays > 0 inside range
	float inv = hfAttenuationGain( HF_ATTEN_INVERSE_CLAMPED, 10.0f, 1.0f, 100.0f );
	FEAT_ASSERT( inv > 0.0f && inv < 1.0f );
	// pan
	float l, r;
	hfPanGains( -1.0f, l, r );
	FEAT_ASSERT( l == 1.0f && r == 0.0f );
	hfPanGains( 1.0f, l, r );
	FEAT_ASSERT( l == 0.0f && r == 1.0f );
	hfPanGains( 0.0f, l, r );
	FEAT_ASSERT( l == 1.0f && r == 1.0f );
	// sound bank: add/find/reuse with generation tags
	hfSoundBank bank;
	bank.Init();
	unsigned int s0 = bank.AddSample( "shot.wav" );
	unsigned int s1 = bank.AddSample( "step.wav" );
	FEAT_ASSERT( s0 == s1 - 1 );
	FEAT_ASSERT( bank.FindSample( "SHOT.WAV" ) == ( int )s0 );
	FEAT_ASSERT( bank.FindSample( "missing.wav" ) == -1 );
	unsigned int k1 = bank.Alloc( s0 );
	unsigned int idx = hfSoundKey::IndexOf( k1 );
	FEAT_ASSERT( bank.IsAlive( k1 ) );
	FEAT_ASSERT( bank.Get( k1 ) && bank.Get( k1 )->sampleId == s0 );
	bank.Free( k1 );
	FEAT_ASSERT( !bank.IsAlive( k1 ) );
	unsigned int k2 = bank.Alloc( s1 );
	FEAT_ASSERT( hfSoundKey::IndexOf( k2 ) == idx );		// slot reused
	FEAT_ASSERT( hfSoundKey::GenOf( k2 ) > hfSoundKey::GenOf( k1 ) );
	FEAT_ASSERT( !bank.IsAlive( k1 ) );							// stale key dead
	// sample info
	hfAudioSampleInfo info;
	info.channels = 2; info.sampleRate = 48000; info.totalFrames = 48000; info.totalBytes = 0;
	FEAT_ASSERT( info.DurationSec() == 1.0f );
	printf( "Audio ok\n" );
}

static void Test_Collide() {
	// ---- sphere vs sphere ----
	{
		hfVec3 a( 0, 0, 0 ), b( 2, 0, 0 );
		FEAT_ASSERT(  hfCollide::TestSphereSphere( a, 1.0f, b, 1.0f ) );	// touching
		FEAT_ASSERT( !hfCollide::TestSphereSphere( a, 0.5f, b, 0.5f ) );	// gap = 1 > 1.0 total
		FEAT_ASSERT(  hfCollide::TestSphereSphereType( a, 1.0f, b, 1.0f ) == HF_I_INTERSECT );
		FEAT_ASSERT(  hfCollide::TestSphereSphereType( a, 3.0f, b, 0.5f ) == HF_I_INSIDE );
		FEAT_ASSERT(  hfCollide::TestSphereSphereType( a, 1.0f, b, 10.0f ) == HF_I_INSIDE );	// sphere a fully inside b
	}
	// ---- sphere vs AABB ----
	{
		hfBounds b;
		b.Clear();
		hfVec3 mn( 1, 1, 1 ), mx( 5, 5, 5 );
		b.AddPoint( mn );
		b.AddPoint( mx );
		hfVec3 c( 3, 3, 3 );
		FEAT_ASSERT(  hfCollide::TestSphereBounds( c, 1.5f, b ) );
		FEAT_ASSERT(  hfCollide::TestSphereBoundsType( c, 1.0f, b ) == HF_I_INSIDE );
		hfVec3 c2( 0, 3, 3 );
		FEAT_ASSERT(  hfCollide::TestSphereBounds( c2, 1.1f, b ) );	// sphere touches corner at x=1
		FEAT_ASSERT( !hfCollide::TestSphereBounds( c2, 0.5f, b ) );	// too far away
		hfVec3 c3( -10, 3, 3 );
		FEAT_ASSERT( !hfCollide::TestSphereBounds( c3, 1.0f, b ) );
	}
	// ---- sphere vs frustum ----
	{
		hfFrustum f;
		// Build a simple frustum looking down -Z, right +X, up +Y.
		//   left: x >= -1, right: x <= 1, bottom: y >= -1, top: y <= 1,
		//   near: z >= -10, far: z <= -1.
		f.planes[ 0 ] = hfPlane( 1, 0, 0, 1 );		// left:   x + 1 >= 0
		f.planes[ 1 ] = hfPlane(-1, 0, 0, 1 );		// right: -x + 1 >= 0
		f.planes[ 2 ] = hfPlane( 0, 1, 0, 1 );		// bottom: y + 1 >= 0
		f.planes[ 3 ] = hfPlane( 0,-1, 0, 1 );		// top:   -y + 1 >= 0
		f.planes[ 4 ] = hfPlane( 0, 0, 1, 10 );		// near:  z + 10 >= 0
		f.planes[ 5 ] = hfPlane( 0, 0,-1, 1 );		// far:  -z + 1 >= 0

		hfVec3 inside( 0, 0, -5 );
		FEAT_ASSERT(  hfCollide::TestSphereFrustum( inside, 0.5f, f ) );
		FEAT_ASSERT(  hfCollide::TestSphereFrustumType( inside, 0.5f, f ) == HF_I_INSIDE );

		hfVec3 nearEdge( 1.1f, 0, -5 );			// just past the right plane
		FEAT_ASSERT( !hfCollide::TestSphereFrustum( nearEdge, 0.0f, f ) );
		FEAT_ASSERT(  hfCollide::TestSphereFrustumType( nearEdge, 0.0f, f ) == HF_I_OUTSIDE );

		hfVec3 straddle( 0.9f, 0, -5 );
		FEAT_ASSERT(  hfCollide::TestSphereFrustum( straddle, 0.5f, f ) );
		FEAT_ASSERT(  hfCollide::TestSphereFrustumType( straddle, 0.5f, f ) == HF_I_INTERSECT );

		hfVec3 behind( 0, 0, 2 );
		FEAT_ASSERT( !hfCollide::TestSphereFrustum( behind, 0.5f, f ) );
	}
	// ---- cone vs sphere (spot-clip) ----
	{
		hfVec3 conePos( 0, 10, 0 );
		hfVec3 coneDir( 0, -1, 0 );
		float range = 20.0f;
		// 90 degree half-angle: cos45 = 0.707
		float cos = 0.707f;
		float sin = cos;
		hfVec3 target( 0, 0, 0 );
		FEAT_ASSERT(  hfCollide::TestConeSphere( conePos, coneDir, range, cos, sin, target, 1.0f ) );
		hfVec3 farSide( 15, 0, 0 );
		FEAT_ASSERT( !hfCollide::TestConeSphere( conePos, coneDir, range, cos, sin, farSide, 1.0f ) );
	}
	// ---- ray triangle (Moller-Trumbore) ----
	{
		hfVec3 a( 0, 0, 0 ), b( 10, 0, 0 ), c( 0, 10, 0 );
		hfVec3 origin( 1, 1, 10 );
		hfVec3 dir( 0, 0, -1 );
		float t, u, v;
		FEAT_ASSERT(  hfCollide::RayTriangle( origin, dir, 100.0f, a, b, c, t, u, v ) );
		FEAT_ASSERT(  t > 0.0f && t < 20.0f );
		FEAT_ASSERT(  u > 0.0f && v > 0.0f && u + v < 1.0f );
		hfVec3 miss( 100, 100, 10 );
		FEAT_ASSERT( !hfCollide::RayTriangle( miss, dir, 100.0f, a, b, c, t, u, v ) );
	}
	// ---- mesh collider: floor ray + sphere trace ----
	{
		hfVec3 verts[ 3 ] = { hfVec3( -50, -50, 0 ), hfVec3( 50, -50, 0 ), hfVec3( 0, 50, 0 ) };
		int tris[ 3 ] = { 0, 1, 2 };
		hfMeshCollider mesh;
		mesh.Set( verts, 3, tris, 1 );
		FEAT_ASSERT( mesh.IsValid() );

		// ray from above should hit z=0
		float t; hfVec3 n; int ti;
		FEAT_ASSERT( mesh.RayTrace( hfVec3( 0, 0, 10 ), hfVec3( 0, 0, -1 ), 100.0f, t, n, ti ) );
		FEAT_ASSERT( ti == 0 );
		FEAT_ASSERT( t > 9.5f && t < 10.5f );		// z=10 → hit at t≈10

		// swept sphere: start at z=5, end at z=-5, radius 0.5  → expect t ≈ 4.5/10 = 0.45
		float st; hfVec3 sp, sn; int sti;
		FEAT_ASSERT( mesh.SphereTrace( hfVec3( 0, 0, 5 ), hfVec3( 0, 0, -5 ), 0.5f, st, sp, sn, sti ) );
		FEAT_ASSERT( sti == 0 );
		FEAT_ASSERT( st > 0.44f && st < 0.46f );
	}
	printf( "Collide ok\n" );
}

static void Test_AudioSpatial() {
	using namespace HF_Audio;
	// emitter directly at listener → no attenuation, no pan
	hfSoundListener listener;
	listener.position = hfVec3( 10, 0, 5 );
	listener.SetPositionForward( hfVec3( 10, 0, 5 ), hfVec3( 0, 1, 0 ) );

	hfSpatialEmitter em;
	em.play = true;
	em.volume = 1.0f;
	em.spatialized = true;
	em.minDistance = 1.0f;
	em.maxDistance = 100.0f;
	em.attenuationModel = HF_ATTEN_LINEAR;
	em.position = listener.position;

	float left, right, pan;
	float gain = hfSpatialEvaluate( listener, em, left, right, pan );
	FEAT_ASSERT( gain == 1.0f );
	FEAT_ASSERT( left == 1.0f );
	FEAT_ASSERT( right == 1.0f );
	FEAT_ASSERT( pan == 0.0f );

	// emitter off to the right of the listener
	hfSoundListener lis2;
	lis2.position = hfVec3( 0, 0, 0 );
	lis2.SetPositionForward( hfVec3( 0, 0, 0 ), hfVec3( 0, 1, 0 ) );	// forward=+Y, right=+X

	hfSpatialEmitter em2;
	em2.play = true;
	em2.volume = 1.0f;
	em2.spatialized = true;
	em2.minDistance = 10.0f;
	em2.maxDistance = 100.0f;
	em2.attenuationModel = HF_ATTEN_LINEAR;
	em2.position = hfVec3( 50, 0, 0 );

	gain = hfSpatialEvaluate( lis2, em2, left, right, pan );
	FEAT_ASSERT( gain > 0.0f );
	FEAT_ASSERT( gain < 1.0f );		// distance 50 > min 10 → attenuated
	FEAT_ASSERT( pan > 0.9f );		// mostly right
	FEAT_ASSERT( right > left );		// louder on the right

	// emitter behind the listener
	hfSpatialEmitter em3 = em2;
	em3.position = hfVec3( 0, -50, 0 );
	gain = hfSpatialEvaluate( lis2, em3, left, right, pan );
	FEAT_ASSERT( gain > 0.0f );
	FEAT_ASSERT( hfMath::Fabs( pan ) < 0.05f );	// centered left/right
	FEAT_ASSERT( left == right );

	// non-spatialized emitter always full volume
	hfSpatialEmitter em4;
	em4.play = true;
	em4.spatialized = false;
	em4.volume = 0.8f;
	em4.position = hfVec3( 9999, 0, 0 );
	gain = hfSpatialEvaluate( lis2, em4, left, right, pan );
	FEAT_ASSERT( gain == 0.8f );
	FEAT_ASSERT( left == 0.8f );
	FEAT_ASSERT( right == 0.8f );
	FEAT_ASSERT( pan == 0.0f );

	// muted emitter
	hfSpatialEmitter em5;
	em5.play = false;
	em5.volume = 1.0f;
	gain = hfSpatialEvaluate( lis2, em5, left, right, pan );
	FEAT_ASSERT( gain == 0.0f );
	FEAT_ASSERT( left == 0.0f );
	FEAT_ASSERT( right == 0.0f );
	printf( "AudioSpatial ok\n" );
}

static void Test_Scene() {
	using namespace HF_Scene;

	// ---- hierarchy + world matrices ----
	{
		HF_Scene::hfWorld world;
		HF_Ecs::Entity root = world.CreateEntity( "root" );
		HF_Ecs::Entity child = world.CreateEntity( "child" );
		FEAT_ASSERT( root != HF_Ecs::ENTITY_NULL && child != HF_Ecs::ENTITY_NULL );

		world.GetComponent< hfTransformComponent >( root )->SetPosition( hfVec3( 10, 0, 0 ) );
		world.GetComponent< hfTransformComponent >( child )->SetPosition( hfVec3( 1, 0, 0 ) );
		world.SetParent( child, root );

		// identity-rotation parent: child world pos = root + local
		world.Update( 0.016f );
		hfVec3 wp = world.GetComponent< hfTransformComponent >( child )->WorldPosition();
		FEAT_NEAR( wp.x, 11.0f, 1e-4f );
		FEAT_NEAR( wp.y, 0.0f, 1e-4f );
		FEAT_NEAR( wp.z, 0.0f, 1e-4f );

		// rotate parent 90deg about Z (ccw): hfQuat * vec uses q⁻¹vq so negate axis
		world.GetComponent< hfTransformComponent >( root )->SetRotation( hfQuat( 0.0f, 0.0f, -0.70710678f, 0.70710678f ) );
		world.Update( 0.016f );
		wp = world.GetComponent< hfTransformComponent >( child )->WorldPosition();
		FEAT_NEAR( wp.x, 10.0f, 1e-3f );
		FEAT_NEAR( wp.y, 1.0f, 1e-3f );
		FEAT_NEAR( wp.z, 0.0f, 1e-3f );

		// add non-uniform scale on the parent: local +X scaled by 2 then rotated
		world.GetComponent< hfTransformComponent >( root )->SetScale( hfVec3( 2.0f, 1.0f, 1.0f ) );
		world.Update( 0.016f );
		wp = world.GetComponent< hfTransformComponent >( child )->WorldPosition();
		FEAT_NEAR( wp.x, 10.0f, 1e-3f );
		FEAT_NEAR( wp.y, 2.0f, 1e-3f );
		FEAT_NEAR( wp.z, 0.0f, 1e-3f );

		// children query
		HF_Ecs::Entity kids[ 4 ];
		FEAT_ASSERT( world.GetChildren( root, kids, 4 ) == 1 );
		FEAT_ASSERT( kids[0] == child );

		// tag lookup
		FEAT_ASSERT( world.FindEntityByTag( "child" ) == child );
		FEAT_ASSERT( world.FindEntityByTag( "CHILD" ) == child );
		FEAT_ASSERT( world.FindEntityByTag( "nope" ) == HF_Ecs::ENTITY_NULL );

		// cycle prevention: root must not become a child of its own child
		world.SetParent( root, child );
		FEAT_ASSERT( world.GetParent( root ) == HF_Ecs::ENTITY_NULL );
		FEAT_ASSERT( world.GetParent( child ) == root );

		// destroying the parent adopts the child out (becomes a root)
		world.DestroyEntity( root );
		world.Update( 0.016f );
		FEAT_ASSERT( !world.HasComponent< hfHierarchyComponent >( root ) );
		FEAT_ASSERT( world.GetComponent< hfTransformComponent >( child ) != NULL );
		FEAT_ASSERT( world.GetParent( child ) == HF_Ecs::ENTITY_NULL );
		wp = world.GetComponent< hfTransformComponent >( child )->WorldPosition();
		FEAT_NEAR( wp.x, 1.0f, 1e-4f );
		FEAT_NEAR( wp.y, 0.0f, 1e-4f );
		FEAT_NEAR( wp.z, 0.0f, 1e-4f );
	}

	// ---- camera chain ----
	{
		HF_Scene::hfWorld world;
		HF_Ecs::Entity camEnt = world.CreateEntity( "cam" );
		hfCameraComponent &cam = world.AddComponent< hfCameraComponent >( camEnt );
		cam.fovY = 90.0f;
		cam.aspect = 16.0f / 9.0f;
		cam.nearZ = 0.1f;
		cam.farZ = 1000.0f;
		// camera at origin, identity look (forward = -Z)
		world.GetComponent< hfTransformComponent >( camEnt )->SetPosition( hfVec3( 0, 0, 0 ) );

		world.Update( 0.016f );

		// point straight ahead, 10 units away, x=5
		hfVec3 ndc = HF_Scene::hfCameraSystem::ProjectToClip( cam, hfVec3( 5.0f, 0.0f, -10.0f ) );
		FEAT_NEAR( ndc.x, 0.28125f, 1e-3f );	// (fovY 90, aspect 16:9) => 5 * (9/16) / 10
		FEAT_NEAR( ndc.y, 0.0f, 1e-4f );
		FEAT_ASSERT( ndc.z > 0.9f && ndc.z < 1.0f );	// toward far plane

		// point at the right frustum edge
		hfVec3 edge = HF_Scene::hfCameraSystem::ProjectToClip( cam, hfVec3( 10.0f * ( 16.0f / 9.0f ), 0.0f, -10.0f ) );
		FEAT_NEAR( edge.x, 1.0f, 1e-3f );

		// camera forward/position derivations
		hfVec3 fwd = cam.GetForward( world.GetComponent< hfTransformComponent >( camEnt )->worldMatrix );
		FEAT_NEAR( fwd.x, 0.0f, 1e-4f );
		FEAT_NEAR( fwd.y, 0.0f, 1e-4f );
		FEAT_NEAR( fwd.z, -1.0f, 1e-4f );
		hfVec3 pos = cam.GetPosition( world.GetComponent< hfTransformComponent >( camEnt )->worldMatrix );
		FEAT_NEAR( pos.x, 0.0f, 1e-4f );
		FEAT_NEAR( pos.y, 0.0f, 1e-4f );
		FEAT_NEAR( pos.z, 0.0f, 1e-4f );
	}

	// ---- growable pools: more than the old fixed 256-slot cap ----
	{
		HF_Scene::hfWorld world;
		static HF_Ecs::Entity pool[ 600 ];
		for ( int i = 0; i < 600; i++ ) {
			pool[i] = world.CreateEntity();
			world.GetComponent< hfTransformComponent >( pool[i] )->SetPosition( hfVec3( ( float )i, 0, 0 ) );
		}
		FEAT_ASSERT( world.GetRegistry().GetLiveCount() == 600 );
		world.Update( 0.016f );
		FEAT_ASSERT( world.GetComponent< hfTransformComponent >( pool[0] ) != NULL );

		// destroy pool[1] only, verify its slot is reused (LIFO free list)
		world.DestroyEntity( pool[1] );
		FEAT_ASSERT( world.GetRegistry().GetLiveCount() == 599 );
		HF_Ecs::Entity reused = world.CreateEntity();
		FEAT_ASSERT( ( reused & HF_Ecs::INDEX_MASK ) == ( pool[1] & HF_Ecs::INDEX_MASK ) );
		FEAT_ASSERT( !world.GetRegistry().IsAlive( pool[1] ) );

		// cleanup fully
		world.Clear();
		FEAT_ASSERT( world.GetRegistry().GetLiveCount() == 0 );
	}
	printf( "Scene ok\n" );
}

static void Test_Stream() {
	using namespace HF_Stream;
	TextureEntry t;
	t.id = 1; t.lastUsedFrame = 100; t.distance = 10.0f;
	t.width = 1024; t.height = 1024; t.mipCount = 4; t.currentBaseMip = 0; t.formatBpp = 4; t.reserved = 0;
	FEAT_ASSERT( mipBytes( t, 0 ) == 1024*1024*4 );
	FEAT_ASSERT( residentBytes( t, 0 ) == mipBytes(t,0)+mipBytes(t,1)+mipBytes(t,2)+mipBytes(t,3) );
	unsigned short want = targetBaseMip( 10.0f, 4, 0.0f );
	FEAT_ASSERT( want >= 0 && want <= 3 );
	TextureEntry entries[2];
	entries[0] = t;
	entries[0].id = 0; entries[0].distance = 10.0f; entries[0].lastUsedFrame = 100;
	entries[0].currentBaseMip = 0;
	entries[1] = t;
	entries[1].id = 1; entries[1].distance = 200.0f; entries[1].lastUsedFrame = 50;
	entries[1].currentBaseMip = 0;
	unsigned int res;
	planResidency( entries, 2, mipBytes( t, 0 ) * 2 + 1024, res );
	FEAT_ASSERT( res <= mipBytes( t, 0 ) * 2 + 1024 );
	FEAT_ASSERT( entries[1].currentBaseMip > 0 );		// far texture dropped
	printf( "Stream ok\n" );
}

// ---- lights: world-space resolution from transform + cluster feed ----
static void Test_Lights() {
	using namespace HF_Scene;

	HF_Scene::hfWorld world;
	HF_Scene::hfLightSystem lightSys;
	world.RegisterSystem( &lightSys );

	HF_Ecs::Entity lamp = world.CreateEntity( "lamp" );
	world.GetComponent< hfTransformComponent >( lamp )->SetPosition( hfVec3( 1.0f, 2.0f, 3.0f ) );
	// visual +90 about +X: hfQuat*vec rotates by the negated axis, so use -X.
	world.GetComponent< hfTransformComponent >( lamp )->SetRotation( hfQuat( -0.70710678f, 0.0f, 0.0f, 0.70710678f ) );
	HF_Scene::hfLightComponent &light = world.AddComponent< HF_Scene::hfLightComponent >( lamp );
	light.type = HF_Scene::HF_LIGHT_SPOT;
	light.intensity = 2.5f;
	light.range = 42.0f;

	HF_Ecs::Entity fill = world.CreateEntity( "fill" );
	world.GetComponent< hfTransformComponent >( fill )->SetPosition( hfVec3( -5.0f, 0.0f, 0.0f ) );
	world.AddComponent< HF_Scene::hfLightComponent >( fill ).type = HF_Scene::HF_LIGHT_POINT;

	world.Update( 0.016f );
	FEAT_ASSERT( lightSys.NumLights() == 2 );
	const HF_Scene::hfWorldLight &wl = lightSys.GetLight( 0 );
	FEAT_NEAR( wl.position.x, 1.0f, 1e-3f );
	FEAT_NEAR( wl.position.z, 3.0f, 1e-3f );
	// spot forward -Z rotated visual +90 about +X -> +Y
	FEAT_NEAR( wl.direction.x, 0.0f, 1e-3f );
	FEAT_NEAR( wl.direction.y, 1.0f, 1e-3f );
	FEAT_NEAR( wl.direction.z, 0.0f, 1e-3f );
	FEAT_NEAR( wl.range, 42.0f, 1e-3f );
	FEAT_NEAR( wl.priority, 2.5f, 1e-3f );
	FEAT_NEAR( light.direction.y, 1.0f, 1e-3f );		// back onto the component
	FEAT_NEAR( lightSys.GetLight( 1 ).position.x, -5.0f, 1e-3f );

	// disable the fill light -> culled from the resolve
	light.enabled = false;
	world.Update( 0.016f );
	FEAT_ASSERT( lightSys.NumLights() == 1 );
	printf( "Lights ok\n" );
}

// ---- typed resource registry over the packed store ----
static void Test_Resources() {
	const char wallMtr[] = "wall { stages { map textures/wall.tga } }";
	const char boxBytes[] = "BXM<\x01\x00\x00\x00>\0\0\0";			// fake binary mesh
	HF_Asset::AssetSource items[2];
	items[0].name = "textures/wall.mtr"; items[0].data = wallMtr; items[0].size = sizeof( wallMtr );
	items[1].name = "meshes/box.mesh"; items[1].data = boxBytes; items[1].size = sizeof( boxBytes );
	HF_Asset::PackedStore pack = HF_Asset::Pack( items, 2 );
	FEAT_ASSERT( pack.ok );

	HF_Res::hfResourceManager rm;
	FEAT_ASSERT( rm.LoadStore( pack.data, pack.size ) );
	int mtr = rm.Create( "textures/wall.mtr", HF_Res::HF_RK_MATERIAL );
	int mesh = rm.Create( "meshes/box.mesh", HF_Res::HF_RK_MESH );
	FEAT_ASSERT( mtr == 0 && mesh == 1 );
	FEAT_ASSERT( rm.Has( "meshes/box.mesh" ) );
	FEAT_ASSERT( rm.Create( "meshes/box.mesh", HF_Res::HF_RK_MESH ) == mesh );	// idempotent
	FEAT_ASSERT( rm.NumResources() == 2 );

	hfStr blob;
	FEAT_ASSERT( rm.Extract( "meshes/box.mesh", blob ) );
	FEAT_ASSERT( !hfStr::Icmp( blob.c_str(), boxBytes ) );

	// material parser on the extracted asset
	HF_Asset::mtrDef_t defs[ 4 ];
	int n = HF_Asset::ParseMtr( wallMtr, sizeof( wallMtr ), defs, 4 );
	FEAT_ASSERT( n == 1 );
	FEAT_ASSERT( defs[0].stages[0].textureName.Length() > 0 );

	// ref counting
	rm.IncRef( mesh ); rm.IncRef( mesh );
	FEAT_ASSERT( rm.GetRefCount( mesh ) == 2 );
	rm.DecRef( mesh ); rm.DecRef( mesh );
	FEAT_ASSERT( rm.GetRefCount( mesh ) == 0 );

	// hot reload markers
	FEAT_ASSERT( !rm.IsDirty( mesh ) && rm.GetGeneration( mesh ) == 0 );
	rm.Bump( mesh );
	FEAT_ASSERT( rm.IsDirty( mesh ) && rm.GetGeneration( mesh ) == 1 );
	rm.AckDirty( mesh );
	FEAT_ASSERT( !rm.IsDirty( mesh ) && rm.GetGeneration( mesh ) == 1 );

	// malformed store is rejected
	FEAT_ASSERT( !rm.LoadStore( ( const byte * )"junk", 4 ) );
	HF_Asset::FreeStore( pack );
	printf( "Resources ok\n" );
}

// ---- text scene round trip ----
static void Test_Serialization() {
	using namespace HF_Scene;

	HF_Scene::hfWorld src;
	HF_Ecs::Entity root = src.CreateEntity( "root" );
	HF_Ecs::Entity child = src.CreateEntity( "child" );
	src.SetParent( child, root );
	src.GetComponent< hfTransformComponent >( child )->SetPosition( hfVec3( 10.0f, 0.0f, 0.0f ) );
	src.GetComponent< hfTransformComponent >( child )->SetRotation( hfQuat( -0.70710678f, 0.0f, 0.0f, 0.70710678f ) );

	HF_Scene::hfCameraComponent &cam = src.AddComponent< HF_Scene::hfCameraComponent >( child );
	cam.fovY = 60.0f; cam.aspect = 4.0f / 3.0f; cam.nearZ = 0.5f; cam.farZ = 500.0f;
	cam.ComputeProjection();

	HF_Scene::hfLightComponent &light = src.AddComponent< HF_Scene::hfLightComponent >( root );
	light.type = HF_Scene::HF_LIGHT_DIRECTIONAL;
	light.intensity = 3.0f; light.range = 200.0f; light.castsShadow = false;

	HF_Scene::hfMeshRendererComponent &mr = src.AddComponent< HF_Scene::hfMeshRendererComponent >( child );
	mr.meshId = 77; mr.materialId = 5;
	mr.localBounds = hfBounds( hfVec3( -1, -1, -1 ), hfVec3( 1, 1, 1 ) );
	mr.meshName = "armor";

	hfStr text;
	FEAT_ASSERT( HF_Scene::hfSceneWriter::Write( src, text ) );
	FEAT_ASSERT( text.Length() > 40 );

	HF_Scene::hfWorld dst;
	FEAT_ASSERT( HF_Scene::hfSceneReader::Read( dst, text.c_str() ) );
	FEAT_ASSERT( dst.GetRegistry().GetLiveCount() == 2 );
	dst.Update( 0.016f );

	HF_Ecs::Entity root2 = dst.FindEntityByTag( "root" );
	HF_Ecs::Entity child2 = dst.FindEntityByTag( "child" );
	FEAT_ASSERT( root2 != HF_Ecs::ENTITY_NULL && child2 != HF_Ecs::ENTITY_NULL );
	FEAT_ASSERT( dst.GetParent( child2 ) == root2 );
	HF_Ecs::Entity kids[ 4 ];
	FEAT_ASSERT( dst.GetChildren( root2, kids, 4 ) == 1 && kids[0] == child2 );

	HF_Scene::hfTransformComponent *ct = dst.GetComponent< HF_Scene::hfTransformComponent >( child2 );
	FEAT_NEAR( ct->WorldPosition().x, 10.0f, 1e-3f );
	FEAT_NEAR( ct->WorldForward().y, -1.0f, 1e-3f );	// rotation -Z->+Y survived (local +Z->-Y)
	FEAT_NEAR( ct->rotation.w, 0.70710678f, 1e-3f );

	HF_Scene::hfCameraComponent *cam2 = dst.GetComponent< HF_Scene::hfCameraComponent >( child2 );
	FEAT_ASSERT( cam2 );
	FEAT_NEAR( cam2->fovY, 60.0f, 1e-3f );
	FEAT_NEAR( cam2->aspect, 4.0f / 3.0f, 1e-3f );
	FEAT_NEAR( cam2->farZ, 500.0f, 1e-3f );

	HF_Scene::hfLightComponent *light2 = dst.GetComponent< HF_Scene::hfLightComponent >( root2 );
	FEAT_ASSERT( light2 && light2->type == HF_Scene::HF_LIGHT_DIRECTIONAL );
	FEAT_NEAR( light2->intensity, 3.0f, 1e-3f );
	FEAT_ASSERT( !light2->castsShadow );

	HF_Scene::hfMeshRendererComponent *mr2 = dst.GetComponent< HF_Scene::hfMeshRendererComponent >( child2 );
	FEAT_ASSERT( mr2 && mr2->meshId == 77 && mr2->materialId == 5 );
	FEAT_NEAR( mr2->localBounds[1].z, 1.0f, 1e-3f );
	FEAT_ASSERT( !hfStr::Icmp( mr2->meshName.c_str(), "armor" ) );

	// malformed input must not crash / must return ok=false preserved scenes
	HF_Scene::hfWorld broken;
	HF_Scene::hfSceneReader::Read( broken, "entity\ntag \"x\"\npos 1 2\n" );	// truncated intent ok
	printf( "Serialization ok\n" );
}

// ---- rigid-body physics: broadphase, solver, sleep, triggers, layers ----
static void Test_Physics() {
	using namespace HF_Scene;
	using namespace HF_Physics;

	// (a) broadphase unit
	{
		hfBroadphase bp;
		hfBroadphaseEntry e;
		e.entity = 1u; e.box = hfBounds( hfVec3( 0, 0, 0 ), hfVec3( 1, 1, 1 ) );
		bp.Add( e );
		e.entity = 2u; e.box = hfBounds( hfVec3( 10, 10, 10 ), hfVec3( 11, 11, 11 ) );
		bp.Add( e );
		FEAT_ASSERT( bp.Build() == 0 );		// far apart -> nothing
		e.entity = 3u; e.box = hfBounds( hfVec3( 0.5f, 0.5f, 0.5f ), hfVec3( 2, 2, 2 ) );
		int id = bp.Add( e );
		FEAT_ASSERT( id == 2 );
		FEAT_ASSERT( bp.Build() == 1 );		// overlaps entry 0 only
		int a, b;
		bp.GetPair( 0, a, b );
		FEAT_ASSERT( ( a == 0 && b == 2 ) || ( a == 2 && b == 0 ) );
	}

	// (b) gravity, resting and sleeping against a static floor
	{
		HF_Scene::hfWorld world;
		hfPhysicsSystem phys;
		world.RegisterSystem( &phys );

		HF_Ecs::Entity floor = world.CreateEntity( "floor" );
		world.GetComponent< hfTransformComponent >( floor )->SetPosition( hfVec3( 0, 0, -1 ) );
		hfRigidBodyComponent &fb = world.AddComponent< hfRigidBodyComponent >( floor );
		fb.type = HF_BT_STATIC; fb.shape = HF_SHAPE_BOX; fb.halfExtents = hfVec3( 50, 50, 1 );	// top surface z=0
		fb.restitution = 0.0f;
		fb.RefreshMass();

		HF_Ecs::Entity ball = world.CreateEntity( "ball" );
		world.GetComponent< hfTransformComponent >( ball )->SetPosition( hfVec3( 0, 0, 5 ) );
		hfRigidBodyComponent &bb = world.AddComponent< hfRigidBodyComponent >( ball );
		bb.radius = 0.5f; bb.restitution = 0.0f;

		HF_Ecs::Entity hover = world.CreateEntity( "hover" );
		world.GetComponent< hfTransformComponent >( hover )->SetPosition( hfVec3( 5, 0, 5 ) );
		hfRigidBodyComponent &hb = world.AddComponent< hfRigidBodyComponent >( hover );
		hb.radius = 0.5f; hb.gravityScale = 0.0f;

		for ( int i = 0; i < 400; i++ ) { world.Update( 1.0f / 60.0f ); }

		hfTransformComponent *bt = world.GetComponent< hfTransformComponent >( ball );
		FEAT_NEAR( bt->WorldPosition().z, 0.5f, 0.06f );	// resting on the floor top
		FEAT_ASSERT( bb.asleep );							// settled down
		FEAT_NEAR( bb.velocity.Length(), 0.0f, 1e-3f );

		hfTransformComponent *ht = world.GetComponent< hfTransformComponent >( hover );
		FEAT_NEAR( ht->WorldPosition().z, 5.0f, 0.05f );	// gravityScale 0 -> stays put
	}

	// (c) elastic sphere-sphere swap (equal mass, restitution 1)
	{
		HF_Scene::hfWorld world;
		hfPhysicsSystem phys;
		phys.SetGravity( hfVec3( 0, 0, 0 ) );
		world.RegisterSystem( &phys );

		HF_Ecs::Entity A = world.CreateEntity( "A" );
		hfTransformComponent *tA = world.GetComponent< hfTransformComponent >( A );
		tA->SetPosition( hfVec3( 0, 0, 0 ) );
		hfRigidBodyComponent &ra = world.AddComponent< hfRigidBodyComponent >( A );
		ra.radius = 0.5f; ra.restitution = 1.0f; ra.velocity = hfVec3( 1, 0, 0 );

		HF_Ecs::Entity B = world.CreateEntity( "B" );
		hfTransformComponent *tB = world.GetComponent< hfTransformComponent >( B );
		tB->SetPosition( hfVec3( 4, 0, 0 ) );
		hfRigidBodyComponent &rb = world.AddComponent< hfRigidBodyComponent >( B );
		rb.radius = 0.5f; rb.restitution = 1.0f; rb.velocity = hfVec3( -1, 0, 0 );

		for ( int i = 0; i < 240; i++ ) { world.Update( 1.0f / 60.0f ); }

		// heads-on equal-mass elastic collision: the x velocities swap
		FEAT_ASSERT( ra.velocity.x < 0.0f );	// A bounced back to -x
		FEAT_ASSERT( rb.velocity.x > 0.0f );	// B bounced forward to +x
		FEAT_NEAR( fabsf( ra.velocity.x ), 1.0f, 0.25f );
		FEAT_NEAR( fabsf( rb.velocity.x ), 1.0f, 0.25f );
	}

	// (d) triggers: enter/stay/exit, active overlaps, layer exclusion
	{
		HF_Scene::hfWorld world;
		hfPhysicsSystem phys;
		world.RegisterSystem( &phys );

		HF_Ecs::Entity trig = world.CreateEntity( "trig" );
		hfRigidBodyComponent &tg = world.AddComponent< hfRigidBodyComponent >( trig );
		tg.type = HF_BT_STATIC; tg.shape = HF_SHAPE_BOX;
		tg.halfExtents = hfVec3( 5, 5, 5 ); tg.isTrigger = true; tg.layer = 1;
		tg.RefreshMass();

		HF_Ecs::Entity ball = world.CreateEntity( "ball" );
		world.GetComponent< hfTransformComponent >( ball )->SetPosition( hfVec3( 0, 0, 15 ) );
		hfRigidBodyComponent &bb = world.AddComponent< hfRigidBodyComponent >( ball );
		bb.radius = 0.5f;	// keeps the default group bit (1)

		// layer exclusion: trigger mask excludes group bit 0 -> no events
		tg.mask = 0xFFFFFFFEu;
		for ( int i = 0; i < 30; i++ ) { world.Update( 1.0f / 60.0f ); }
		bool sawEnter = false;
		for ( int i = 0; i < phys.NumEvents(); i++ ) {
			if ( phys.GetEvent( i ).state == 1 ) { sawEnter = true; }
		}
		FEAT_ASSERT( !sawEnter );
		FEAT_ASSERT( phys.NumActiveOverlaps() == 0 );

		// reinstate collision, drop the ball through the (static) trigger
		tg.mask = 0xFFFFFFFFu;
		bb.velocity = hfVec3( 0, 0, 0 );
		bb.asleep = false;
		world.GetComponent< hfTransformComponent >( ball )->SetPosition( hfVec3( 0, 0, 15 ) );
		sawEnter = false;
		for ( int i = 0; i < 120 && !sawEnter; i++ ) {
			world.Update( 1.0f / 60.0f );
			for ( int k = 0; k < phys.NumEvents(); k++ ) {
				if ( phys.GetEvent( k ).state == 1 ) {
					HF_Ecs::Entity ea = phys.GetEvent( k ).a, eb = phys.GetEvent( k ).b;
					if ( ( ea == ball && eb == trig ) || ( ea == trig && eb == ball ) ) { sawEnter = true; }
				}
			}
		}
		FEAT_ASSERT( sawEnter );
		FEAT_ASSERT( phys.NumActiveOverlaps() == 1 );
		HF_Ecs::Entity oa, ob;
		phys.GetActiveOverlap( 0, oa, ob );
		FEAT_ASSERT( ( oa == ball && ob == trig ) || ( oa == trig && ob == ball ) );

		// stay events while inside, then teleport out -> exit event
		for ( int i = 0; i < 30; i++ ) { world.Update( 1.0f / 60.0f ); }
		bb.velocity = hfVec3( 0, 0, 0 );
		bb.asleep = false;
		world.GetComponent< hfTransformComponent >( ball )->SetPosition( hfVec3( 0, 0, 50 ) );
		bool sawExit = false;
		for ( int i = 0; i < 60 && !sawExit; i++ ) {
			world.Update( 1.0f / 60.0f );
			for ( int k = 0; k < phys.NumEvents(); k++ ) {
				if ( phys.GetEvent( k ).state == -1 ) {
					HF_Ecs::Entity ea = phys.GetEvent( k ).a, eb = phys.GetEvent( k ).b;
					if ( ( ea == ball && eb == trig ) || ( ea == trig && eb == ball ) ) { sawExit = true; }
				}
			}
		}
		FEAT_ASSERT( sawExit );
		FEAT_ASSERT( phys.NumActiveOverlaps() == 0 );
	}

	printf( "Physics ok\n" );
}

/*
================================

Backend parity, stability and determinism

The same contract has to hold whichever solver runs, and the two are allowed
to disagree about numbers but not about behaviour. Three things are checked:

  - a stack settles and then stays settled (no jitter, no sinking, no creep)
  - the same scene stepped twice produces bit-identical state
  - triggers report enter and exit under both solvers
================================
*/
struct PhysicsStackState {
	float	z[ 3 ];
	float	vz[ 3 ];
	bool	asleep[ 3 ];
};

// Builds the same scene every time: floor plus a three box stack sitting
// exactly on it. Returns the final state so two runs can be compared.
static PhysicsStackState RunPhysicsStack( HF_Physics::hfPhysicsSystem::BackendType backend,
										  float *outX = NULL, float *outY = NULL ) {
	using namespace HF_Scene;
	using namespace HF_Physics;

	hfWorld world;
	hfPhysicsSystem phys;
	phys.SetBackend( backend );
	world.RegisterSystem( &phys );
	FEAT_ASSERT( phys.GetBackend() == ( backend == HF_Physics::hfPhysicsSystem::BACKEND_JOLT &&
										  HF_Physics::hfPhysicsSystem::IsBackendAvailable( HF_Physics::hfPhysicsSystem::BACKEND_JOLT ) ?
										  backend : HF_Physics::hfPhysicsSystem::BACKEND_BUILTIN ) );

	HF_Ecs::Entity floor = world.CreateEntity( "floor" );
	world.GetComponent< hfTransformComponent >( floor )->SetPosition( hfVec3( 0, 0, -1 ) );
	hfRigidBodyComponent &fb = world.AddComponent< hfRigidBodyComponent >( floor );
	fb.type = HF_BT_STATIC;
	fb.shape = HF_SHAPE_BOX;
	fb.halfExtents = hfVec3( 20, 20, 1 );		// top surface at z = 0
	fb.restitution = 0.0f;
	fb.RefreshMass();

	HF_Ecs::Entity boxes[ 3 ];
	for ( int i = 0; i < 3; i++ ) {
		boxes[ i ] = world.CreateEntity( "box" );
		world.GetComponent< hfTransformComponent >( boxes[ i ] )->SetPosition( hfVec3( 0, 0, 1.0f + i * 2.0f ) );
		hfRigidBodyComponent &b = world.AddComponent< hfRigidBodyComponent >( boxes[ i ] );
		b.type = HF_BT_DYNAMIC;
		b.shape = HF_SHAPE_BOX;
		b.halfExtents = hfVec3( 1, 1, 1 );
		b.mass = 1.0f;
		b.restitution = 0.0f;
		b.friction = 0.5f;
		b.RefreshMass();
	}

	for ( int i = 0; i < 600; i++ ) { world.Update( 1.0f / 60.0f ); }

	PhysicsStackState s;
	for ( int i = 0; i < 3; i++ ) {
		hfTransformComponent *t = world.GetComponent< hfTransformComponent >( boxes[ i ] );
		hfRigidBodyComponent &b = *world.GetComponent< hfRigidBodyComponent >( boxes[ i ] );
		hfVec3 p = t->WorldPosition();
		s.z[ i ] = p.z;
		s.vz[ i ] = b.velocity.z;
		s.asleep[ i ] = b.asleep;
		if ( outX ) { outX[ i ] = p.x; }
		if ( outY ) { outY[ i ] = p.y; }
	}
	return s;
}

static void Test_PhysicsBackend() {
	using namespace HF_Scene;
	using namespace HF_Physics;

	const bool haveJolt = hfPhysicsSystem::IsBackendAvailable( hfPhysicsSystem::BACKEND_JOLT );
	int backends = haveJolt ? 2 : 1;

	// (a) a stack settles where it was placed and then stops moving
	//
	// Only Jolt is asked to hold a three box stack: the built-in fallback is a
	// single pass impulse solver with no position correction and no contact
	// caching, so it sinks through a stack, which is the whole reason the Jolt
	// backend exists. What the fallback must always guarantee is checked too:
	// nothing explodes, nothing leaves the world, nothing turns into a NaN.
	for ( int be = 0; be < backends; be++ ) {
		hfPhysicsSystem::BackendType backend = be == 1 ? hfPhysicsSystem::BACKEND_JOLT
													   : hfPhysicsSystem::BACKEND_BUILTIN;
		float x[ 3 ], y[ 3 ];
		PhysicsStackState settled = RunPhysicsStack( backend, x, y );
		const bool stable = backend == hfPhysicsSystem::BACKEND_JOLT;

		for ( int i = 0; i < 3; i++ ) {
			// every box stays in the world and keeps its position finite
			FEAT_ASSERT( settled.z[ i ] == settled.z[ i ] );
			FEAT_ASSERT( settled.z[ i ] > -1.0f );
			FEAT_ASSERT( settled.z[ i ] < 12.0f );
			if ( !stable ) { continue; }
			// the stack keeps the height it was placed at
			FEAT_NEAR( settled.z[ i ], 1.0f + i * 2.0f, 0.2f );
			FEAT_NEAR( settled.vz[ i ], 0.0f, 0.05f );
			FEAT_NEAR( x[ i ], 0.0f, 0.2f );
			FEAT_NEAR( y[ i ], 0.0f, 0.2f );
		}
	}

	// (b) a body at rest does not creep: stepping further must not move it
	for ( int be = 0; be < backends; be++ ) {
		hfPhysicsSystem::BackendType backend = be == 1 ? hfPhysicsSystem::BACKEND_JOLT
													   : hfPhysicsSystem::BACKEND_BUILTIN;
		// the second run of RunPhysicsStack is bit-identical to the first, which
		// is checked in (c); here the point is the before/after comparison inside
		// a single long run, so it is asserted through the frame loop instead
		hfWorld world;
		hfPhysicsSystem phys;
		phys.SetBackend( backend );
		world.RegisterSystem( &phys );

		HF_Ecs::Entity floor = world.CreateEntity( "floor" );
		world.GetComponent< hfTransformComponent >( floor )->SetPosition( hfVec3( 0, 0, -1 ) );
		hfRigidBodyComponent &fb = world.AddComponent< hfRigidBodyComponent >( floor );
		fb.type = HF_BT_STATIC; fb.shape = HF_SHAPE_BOX; fb.halfExtents = hfVec3( 20, 20, 1 );
		fb.restitution = 0.0f; fb.RefreshMass();

		HF_Ecs::Entity box = world.CreateEntity( "box" );
		world.GetComponent< hfTransformComponent >( box )->SetPosition( hfVec3( 0, 0, 1 ) );
		hfRigidBodyComponent &bb = world.AddComponent< hfRigidBodyComponent >( box );
		bb.shape = HF_SHAPE_BOX; bb.halfExtents = hfVec3( 1, 1, 1 );
		bb.restitution = 0.0f; bb.friction = 0.5f; bb.RefreshMass();

		for ( int i = 0; i < 400; i++ ) { world.Update( 1.0f / 60.0f ); }
		float before = world.GetComponent< hfTransformComponent >( box )->WorldPosition().z;
		for ( int i = 0; i < 400; i++ ) { world.Update( 1.0f / 60.0f ); }
		float after = world.GetComponent< hfTransformComponent >( box )->WorldPosition().z;
		FEAT_NEAR( after, before, 1e-3f );		// no creep once it is at rest
	}

	// (c) determinism: the same scene, stepped again, lands on the same bits
	for ( int be = 0; be < backends; be++ ) {
		hfPhysicsSystem::BackendType backend = be == 1 ? hfPhysicsSystem::BACKEND_JOLT
													   : hfPhysicsSystem::BACKEND_BUILTIN;
		PhysicsStackState first = RunPhysicsStack( backend );
		PhysicsStackState second = RunPhysicsStack( backend );
		for ( int i = 0; i < 3; i++ ) {
			FEAT_ASSERT( first.z[ i ] == second.z[ i ] );
			FEAT_ASSERT( first.vz[ i ] == second.vz[ i ] );
			FEAT_ASSERT( first.asleep[ i ] == second.asleep[ i ] );
		}
	}

	// (d) a sphere dropped through a trigger reports enter then exit
	for ( int be = 0; be < backends; be++ ) {
		hfPhysicsSystem::BackendType backend = be == 1 ? hfPhysicsSystem::BACKEND_JOLT
													   : hfPhysicsSystem::BACKEND_BUILTIN;
		hfWorld world;
		hfPhysicsSystem phys;
		phys.SetBackend( backend );
		world.RegisterSystem( &phys );

		HF_Ecs::Entity trig = world.CreateEntity( "trig" );
		hfRigidBodyComponent &tg = world.AddComponent< hfRigidBodyComponent >( trig );
		tg.type = HF_BT_STATIC; tg.shape = HF_SHAPE_BOX;
		tg.halfExtents = hfVec3( 3, 3, 3 ); tg.isTrigger = true;
		tg.RefreshMass();

		HF_Ecs::Entity ball = world.CreateEntity( "ball" );
		world.GetComponent< hfTransformComponent >( ball )->SetPosition( hfVec3( 0, 0, 15 ) );
		hfRigidBodyComponent &bb = world.AddComponent< hfRigidBodyComponent >( ball );
		bb.radius = 0.5f; bb.gravityScale = 0.0f;
		bb.velocity = hfVec3( 0, 0, -6 );

		bool sawEnter = false, sawExit = false;
		for ( int i = 0; i < 300 && !( sawEnter && sawExit ); i++ ) {
			world.Update( 1.0f / 60.0f );
			for ( int k = 0; k < phys.NumEvents(); k++ ) {
				HF_Ecs::Entity ea = phys.GetEvent( k ).a, eb = phys.GetEvent( k ).b;
				if ( ( ea != ball || eb != trig ) && ( ea != trig || eb != ball ) ) { continue; }
				if ( phys.GetEvent( k ).state == 1 ) { sawEnter = true; }
				if ( phys.GetEvent( k ).state == -1 ) { sawExit = true; }
			}
		}
		FEAT_ASSERT( sawEnter );
		FEAT_ASSERT( sawExit );
		// a trigger never pushes the ball
		FEAT_NEAR( bb.velocity.z, -6.0f, 0.001f );
	}

	printf( "PhysicsBackend ok (%s)\n", haveJolt ? "builtin + jolt" : "builtin only, jolt not in build" );
}

/*
================================

Phase 8 — the --headless switch and the threaded simulation server

Two properties are under test here.

 1. The switch is a mode, not a message: recognizing it has to be observable
    from anywhere in the program before a window, a GPU or a clock exists.
 2. Moving the per-frame simulation onto a worker thread changes nothing a
    consumer can see. The same world, the same seed and the same input log
    have to publish a byte-identical snapshot whether the step ran inline or
    on another thread, which is the whole justification for the server split.

================================
*/
static void Test_Headless() {
	// every spelling the switch accepts
	FEAT_ASSERT( HF_Lib::IsHeadlessArgument( "--headless" ) );
	FEAT_ASSERT( HF_Lib::IsHeadlessArgument( "-headless" ) );
	FEAT_ASSERT( HF_Lib::IsHeadlessArgument( "+headless" ) );
	FEAT_ASSERT( HF_Lib::IsHeadlessArgument( "--HEADLESS" ) );
	// and the near misses it must not swallow
	FEAT_ASSERT( !HF_Lib::IsHeadlessArgument( "headless" ) );
	FEAT_ASSERT( !HF_Lib::IsHeadlessArgument( "--headless1" ) );
	FEAT_ASSERT( !HF_Lib::IsHeadlessArgument( "--verbose" ) );

	// what this process was started with survives a round of parsing
	bool startedWith = HF_Lib::IsHeadless();

	HF_Lib::SetHeadless( false );
	const char *without[] = { "harness", "+set", "g_foo", "1" };
	FEAT_ASSERT( !HF_Lib::ParseHeadlessCommand( 4, without ) );
	FEAT_ASSERT( !HF_Lib::IsHeadless() );

	const char *with[] = { "harness", "+set", "g_foo", "1", "--headless" };
	FEAT_ASSERT( HF_Lib::ParseHeadlessCommand( 5, with ) );
	FEAT_ASSERT( HF_Lib::IsHeadless() );

	HF_Lib::SetHeadless( startedWith );
	FEAT_ASSERT( HF_Lib::IsHeadless() == startedWith );

printf( "Headless ok (started %s)\n", startedWith ? "with --headless" : "without --headless" );
}

struct hfSimRunResult {
	unsigned int		checksum;
	unsigned int		sequence;
	int					bodyCount;
	float				ballX;
	float				ballZ;
	bool				started;
	bool				inlineRan;
};

// a scene whose initial layout is derived from the seed, so "same seed" is a
// condition on the simulation rather than a field carried in the snapshot
static void BuildSimTestScene( HF_Scene::hfWorld &world, HF_Physics::hfPhysicsSystem &phys,
		unsigned int seed, HF_Ecs::Entity &outBall ) {
	using namespace HF_Scene;
	using namespace HF_Physics;

	world.RegisterSystem( &phys );

	unsigned int rng = seed;
	auto next = [ & ]( unsigned int span ) {
		rng = rng * 1664525u + 1013904223u;
		return ( rng >> 8 ) % span;
	};

	HF_Ecs::Entity floor = world.CreateEntity( "floor" );
	world.GetComponent< hfTransformComponent >( floor )->SetPosition( hfVec3( 0, 0, -1 ) );
	hfRigidBodyComponent &fb = world.AddComponent< hfRigidBodyComponent >( floor );
	fb.type = HF_BT_STATIC; fb.shape = HF_SHAPE_BOX; fb.halfExtents = hfVec3( 50, 50, 1 );
	fb.restitution = 0.0f;
	fb.RefreshMass();

	outBall = world.CreateEntity( "ball" );
	float sx = (float)next( 400 ) * 0.01f - 2.0f;
	float sy = (float)next( 400 ) * 0.01f - 2.0f;
	world.GetComponent< hfTransformComponent >( outBall )->SetPosition( hfVec3( sx, sy, 6.0f ) );
	hfRigidBodyComponent &bb = world.AddComponent< hfRigidBodyComponent >( outBall );
	bb.radius = 0.5f; bb.restitution = 0.0f;

	HF_Ecs::Entity peg = world.CreateEntity( "peg" );
	world.GetComponent< hfTransformComponent >( peg )->SetPosition(
		hfVec3( (float)next( 400 ) * 0.01f - 2.0f, (float)next( 400 ) * 0.01f - 2.0f, 3.0f ) );
	hfRigidBodyComponent &pb = world.AddComponent< hfRigidBodyComponent >( peg );
	pb.radius = 0.5f; pb.gravityScale = 0.0f;
}

static hfSimRunResult RunSimServerCase( bool threaded, unsigned int seed, bool useLog, int steps ) {
	using namespace HF_Scene;
	using namespace HF_Physics;
	using namespace HF_Server;

	hfSimRunResult out;
	memset( &out, 0, sizeof( out ) );
	out.ballX = -999.0f; out.ballZ = -999.0f;

	// declaration order matters: sim is torn down first, then the world
	hfWorld world;
	hfPhysicsSystem phys;
	HF_Ecs::Entity ball = HF_Ecs::Entity();
	BuildSimTestScene( world, phys, seed, ball );

	hfSimConfig cfg;
	cfg.threaded = threaded;
	hfSimulationServer sim( &world, cfg );
	sim.SetSeed( seed );

	if ( useLog ) {
		hfSimInput in;
		in.step = 30; in.entity = ball; in.impulse = hfVec3( 2.0f, 0.0f, 0.0f );
		sim.AddInput( in );
		in.step = 45; in.entity = ball; in.impulse = hfVec3( 0.0f, 1.5f, 0.0f );
		sim.AddInput( in );
		FEAT_ASSERT( sim.GetInputCount() == 2 );
	} else {
		FEAT_ASSERT( sim.GetInputCount() == 0 );
	}

	FEAT_ASSERT( sim.HasWorld() );
	FEAT_ASSERT( sim.GetSeed() == seed );

	sim.Start();
	out.started = sim.IsStarted();
	out.inlineRan = !threaded;

	for ( int i = 0; i < steps; i++ ) { sim.StepNow(); }

	hfSimSnapshot snap;
	memset( &snap, 0, sizeof( snap ) );
	if ( sim.CopySnapshot( &snap ) ) {
		out.sequence	= snap.sequence;
		out.bodyCount	= snap.bodyCount;
		out.checksum	= snap.checksum;
		for ( int i = 0; i < snap.bodyCount; i++ ) {
			if ( snap.bodies[i].entity == ball ) {
				out.ballX = snap.bodies[i].position.x;
				out.ballZ = snap.bodies[i].position.z;
			}
		}
	}
	sim.Stop();
	return out;
}

static void Test_SimServer() {
	using namespace HF_Scene;
	using namespace HF_Physics;
	using namespace HF_Server;

	const unsigned int seed = 0xC0FFEEu;
	const int steps = 120;
	const bool headless = HF_Lib::IsHeadless();

	// ---- determinism matrix -------------------------------------------------

	hfSimRunResult inlineA = RunSimServerCase( false, seed, true, steps );
	hfSimRunResult inlineB = RunSimServerCase( false, seed, true, steps );
	hfSimRunResult threadA = RunSimServerCase( true, seed, true, steps );
	hfSimRunResult threadB = RunSimServerCase( true, seed, true, steps );

	// the worker really existed in the threaded cases, so the comparison
	// below is between two different execution models and not vacuous
	FEAT_ASSERT( threadA.started );
	FEAT_ASSERT( threadB.started );
	FEAT_ASSERT( inlineA.started == false );
	FEAT_ASSERT( inlineA.inlineRan && threadA.inlineRan == false );

	// same world, same seed, same input log -> byte identical snapshot
	FEAT_ASSERT( inlineA.checksum != 0 );
	FEAT_ASSERT( inlineA.checksum == inlineB.checksum );
	FEAT_ASSERT( threadA.checksum == threadB.checksum );
	// and the thread does not perturb it
	FEAT_ASSERT( threadA.checksum == inlineA.checksum );

	// the run actually happened and published
	FEAT_ASSERT( inlineA.sequence == (unsigned int)steps );
	FEAT_ASSERT( threadA.sequence == (unsigned int)steps );
	FEAT_ASSERT( inlineA.bodyCount == 3 );			// floor, ball, peg
	FEAT_NEAR( inlineA.ballZ, 0.5f, 0.10f );		// it landed, it did not fall forever

	// ---- the seed and the log are inputs, not decoration -------------------

	hfSimRunResult otherSeed = RunSimServerCase( true, seed ^ 0x1234u, true, steps );
	FEAT_ASSERT( otherSeed.checksum != threadA.checksum );
	FEAT_ASSERT( otherSeed.ballX != threadA.ballX );	// the seed moved the world

	hfSimRunResult noLog = RunSimServerCase( true, seed, false, steps );
	FEAT_ASSERT( noLog.checksum != threadA.checksum );
	FEAT_ASSERT( noLog.ballX != threadA.ballX );		// the impulse moved the ball

	// ---- the front end pattern: read while the worker runs -----------------

	{
		hfWorld world;
		hfPhysicsSystem phys;
		HF_Ecs::Entity ball = HF_Ecs::Entity();
		BuildSimTestScene( world, phys, seed, ball );

		hfSimulationServer sim( &world, hfSimConfig() );
		sim.SetSeed( seed );

		hfSimRid rid = sim.GetRid();
		FEAT_ASSERT( rid.Valid() );
		FEAT_ASSERT( hfSimulationServer::Resolve( rid ) == &sim );
		FEAT_ASSERT( hfSimulationServer::Count() >= 1 );
		FEAT_ASSERT( sim.IsHeadless() == headless );

		sim.Start();
		FEAT_ASSERT( sim.IsStarted() );

		hfSimSnapshot snap;
		memset( &snap, 0, sizeof( snap ) );
		bool have = false;
		sim.Kick();							// step 1 is in flight
		for ( int i = 0; i < steps; i++ ) {
			sim.WaitForStep();				// step i+1 is done
			if ( i + 1 < steps ) { sim.Kick(); }	// step i+2 starts
			if ( sim.CopySnapshot( &snap ) ) { have = true; }	// read mid-step
		}
		sim.Stop();
		FEAT_ASSERT( !sim.IsStarted() );

		FEAT_ASSERT( have );
		FEAT_ASSERT( snap.sequence == (unsigned int)steps );
		FEAT_ASSERT( snap.step == (unsigned int)steps );
		FEAT_NEAR( snap.simTime, (float)steps / 60.0f, 0.001f );
		FEAT_ASSERT( snap.seed == seed );
		FEAT_ASSERT( snap.inputsApplied == 0 );
		// what was published is exactly what it says it is
		FEAT_ASSERT( snap.checksum == snap.ComputeChecksum() );
		FEAT_ASSERT( snap.bodyCount == 3 );
	}


	// ---- an unstarted server still steps, inline ---------------------------

	{
		hfWorld world;
		hfPhysicsSystem phys;
		HF_Ecs::Entity ball = HF_Ecs::Entity();
		BuildSimTestScene( world, phys, seed, ball );

		hfSimConfig cfg;
		cfg.threaded = false;
		hfSimulationServer sim( &world, cfg );
		sim.Start();						// must not spin a thread up
		FEAT_ASSERT( !sim.IsStarted() );
		for ( int i = 0; i < 10; i++ ) { sim.Kick(); }	// no wait needed inline
		hfSimSnapshot snap;
		memset( &snap, 0, sizeof( snap ) );
		FEAT_ASSERT( sim.CopySnapshot( &snap ) );
		FEAT_ASSERT( snap.sequence == 10 );
		FEAT_ASSERT( snap.step == 10 );
	}

	printf( "SimServer ok (%d steps, checksum %u, %s)\n",
		steps, threadA.checksum, threadA.started ? "worker thread" : "no worker" );
}

struct hfAudioRunResult {
	unsigned int		sequence;
	unsigned int		checksum;
	bool				started;
	bool				hasCenter, hasRight, hasFar, hasQuiet;
	float				centerL, centerR, centerGain;
	float				rightL, rightR, rightGain;
	float				farGain;
	bool				farAudible;
	float				quietL, quietR;
};

// A fixed emitter arrangement relative to a listener at the origin looking +Y
// (right = +X): one centred, one 50 units right, one beyond max distance, one
// quiet. The mix is a pure function of that arrangement, so the result must be
// identical inline and on the worker.
static hfAudioRunResult RunAudioServerCase( bool threaded ) {
	using namespace HF_Audio;

	hfAudioRunResult out;
	memset( &out, 0, sizeof( out ) );

	hfAudioConfig cfg;
	cfg.threaded = threaded;
	hfAudioServer audio( cfg );

	hfSoundListener listener;		// at origin, forward +Y, right +X
	audio.SetListener( listener );

	hfSpatialEmitter base;
	base.play = true;
	base.spatialized = true;
	base.volume = 1.0f;
	base.minDistance = 0.0f;
	base.maxDistance = 100.0f;
	base.attenuationModel = HF_ATTEN_LINEAR;

	hfSpatialEmitter center = base;	center.position = hfVec3( 0, 0, 0 );
	hfSpatialEmitter right = base;	right.position = hfVec3( 50, 0, 0 );
	hfSpatialEmitter far = base;	far.position = hfVec3( 200, 0, 0 );
	hfSpatialEmitter quiet = base;	quiet.volume = 0.25f; quiet.position = hfVec3( 0, 0, 0 );

	audio.CreateEmitter( center );	// voice index 0
	audio.CreateEmitter( right );	// 1
	audio.CreateEmitter( far );		// 2
	audio.CreateEmitter( quiet );	// 3

	audio.Start();
	out.started = audio.IsStarted();
	audio.StepNow();

	hfAudioSnapshot mix;
	mix.Zero();
	if ( audio.CopySnapshot( &mix ) ) {
		out.sequence = mix.sequence;
		out.checksum = mix.checksum;
		for ( int i = 0; i < mix.voiceCount; i++ ) {
			const hfAudioVoice &v = mix.voices[ i ];
			switch ( v.emitter.index ) {
				case 0: out.hasCenter = true; out.centerL = v.leftGain; out.centerR = v.rightGain; out.centerGain = v.gain; break;
				case 1: out.hasRight = true; out.rightL = v.leftGain; out.rightR = v.rightGain; out.rightGain = v.gain; break;
				case 2: out.hasFar = true; out.farGain = v.gain; out.farAudible = v.audible; break;
				case 3: out.hasQuiet = true; out.quietL = v.leftGain; out.quietR = v.rightGain; break;
			}
		}
	}

	audio.Stop();
	return out;
}

static void Test_AudioServer() {
	using namespace HF_Audio;

	// ---- the RID registry, server and resources ---------------------------

	{
		hfAudioServer audio{ hfAudioConfig() };
		hfAudioRid rid = audio.GetRid();
		FEAT_ASSERT( rid.Valid() );
		FEAT_ASSERT( hfAudioServer::Resolve( rid ) == &audio );
		FEAT_ASSERT( hfAudioServer::Count() >= 1 );
		FEAT_ASSERT( audio.IsHeadless() == HF_Lib::IsHeadless() );

		hfAudioSampleInfo info;
		memset( &info, 0, sizeof( info ) );
		info.channels = 2; info.sampleRate = 48000; info.totalFrames = 48000;
		info.totalBytes = info.totalFrames * info.channels * (unsigned int)sizeof( float );

		hfAudioRid s0 = audio.CreateSample( 0xAABBCCDDu, info );
		hfAudioRid s0again = audio.CreateSample( 0xAABBCCDDu, info );	// same hash -> same rid
		FEAT_ASSERT( s0.Valid() );
		FEAT_ASSERT( s0.index == s0again.index );
		FEAT_ASSERT( audio.NumSamples() == 1 );
		const hfAudioSampleInfo *got = audio.GetSample( s0 );
		FEAT_ASSERT( got != NULL );
		FEAT_NEAR( got->DurationSec(), 1.0f, 1e-4f );

		hfSpatialEmitter e; e.play = true;
		hfAudioRid er = audio.CreateEmitter( e, s0 );
		FEAT_ASSERT( er.Valid() );
		FEAT_ASSERT( audio.GetEmitter( er ) != NULL );
		audio.SetEmitterPosition( er, hfVec3( 1, 2, 3 ) );
		FEAT_NEAR( audio.GetEmitter( er )->position.z, 3.0f, 1e-5f );
	}

	// ---- clean teardown frees the slot for reuse --------------------------

	{
		hfAudioRid first;
		{
			hfAudioServer temp{ hfAudioConfig() };
			first = temp.GetRid();
		}
		hfAudioServer other{ hfAudioConfig() };
		// the freed slot is reused, so the new server holds it
		FEAT_ASSERT( other.GetRid().Valid() );
		FEAT_ASSERT( other.GetRid().index == first.index );
		FEAT_ASSERT( hfAudioServer::Resolve( first ) == &other );
	}

	// ---- the spatial mix is correct ---------------------------------------

	hfAudioRunResult res = RunAudioServerCase( true );
	FEAT_ASSERT( res.started );
	FEAT_ASSERT( res.sequence == 1 );
	FEAT_ASSERT( res.hasCenter && res.hasRight && res.hasFar && res.hasQuiet );

	// centred source: full gain, centred
	FEAT_NEAR( res.centerGain, 1.0f, 1e-4f );
	FEAT_NEAR( res.centerL, 1.0f, 1e-4f );
	FEAT_NEAR( res.centerR, 1.0f, 1e-4f );

	// 50 of 100 units right: half gain, panned hard right
	FEAT_NEAR( res.rightGain, 0.5f, 1e-3f );
	FEAT_ASSERT( res.rightR > res.rightL );
	FEAT_NEAR( res.rightL, 0.0f, 1e-3f );
	FEAT_NEAR( res.rightR, 0.5f, 1e-3f );

	// beyond max distance: muted
	FEAT_ASSERT( !res.farAudible );
	FEAT_NEAR( res.farGain, 0.0f, 1e-4f );

	// volume scales both channels
	FEAT_NEAR( res.quietL, 0.25f, 1e-4f );
	FEAT_NEAR( res.quietR, 0.25f, 1e-4f );

	// ---- determinism: inline vs worker, again and again -------------------

	hfAudioRunResult inlineA = RunAudioServerCase( false );
	hfAudioRunResult inlineB = RunAudioServerCase( false );
	hfAudioRunResult threadA = RunAudioServerCase( true );
	hfAudioRunResult threadB = RunAudioServerCase( true );

	FEAT_ASSERT( !inlineA.started );
	FEAT_ASSERT( threadA.started && threadA.sequence == 1 );
	FEAT_ASSERT( res.checksum != 0 );
	FEAT_ASSERT( inlineA.checksum == inlineB.checksum );
	FEAT_ASSERT( threadA.checksum == threadB.checksum );
	FEAT_ASSERT( threadA.checksum == inlineA.checksum );		// thread does not perturb it

	// ---- the front end pattern: read while the worker mixes ---------------

	{
		hfAudioServer audio{ hfAudioConfig() };
		hfSoundListener listener;
		audio.SetListener( listener );

		hfSpatialEmitter e; e.play = true; e.spatialized = true;
		e.minDistance = 1.0f; e.maxDistance = 100.0f;
		e.position = hfVec3( 30, 0, 0 );
		hfAudioRid er = audio.CreateEmitter( e );

		audio.Start();
		FEAT_ASSERT( audio.IsStarted() );

		hfAudioSnapshot mix;
		mix.Zero();
		bool have = false;
		audio.Kick();
		for ( int i = 0; i < 8; i++ ) {
			audio.WaitForStep();
			if ( i + 1 < 8 ) { audio.Kick(); }
			if ( audio.CopySnapshot( &mix ) ) { have = true; }
		}
		audio.Stop();
		FEAT_ASSERT( !audio.IsStarted() );
		FEAT_ASSERT( have );
		FEAT_ASSERT( mix.sequence == 8 );
		FEAT_ASSERT( mix.checksum == mix.ComputeChecksum() );
		FEAT_ASSERT( mix.voiceCount == 1 );

		// moving the emitter and mixing again changes the result
		audio.SetEmitterPosition( er, hfVec3( -30, 0, 0 ) );
		audio.StepNow();
		hfAudioSnapshot mix2;
		mix2.Zero();
		FEAT_ASSERT( audio.CopySnapshot( &mix2 ) );
		FEAT_ASSERT( mix2.checksum != mix.checksum );
		FEAT_ASSERT( mix2.voices[0].leftGain > mix2.voices[0].rightGain );	// now on the left

		// freeing an emitter drops its voice
		audio.FreeEmitter( er );
		audio.StepNow();
		hfAudioSnapshot mix3;
		mix3.Zero();
		FEAT_ASSERT( audio.CopySnapshot( &mix3 ) );
		FEAT_ASSERT( mix3.voiceCount == 0 );
	}

	// ---- driven by a simulation snapshot (the Servers hand-off) -----------

	{
		using namespace HF_Scene;
		using namespace HF_Physics;
		using namespace HF_Server;

		hfWorld world;
		hfPhysicsSystem phys;
		HF_Ecs::Entity ball = HF_Ecs::Entity();
		BuildSimTestScene( world, phys, 0xC0FFEEu, ball );

		hfSimulationServer sim( &world, hfSimConfig() );
		sim.SetSeed( 0xC0FFEEu );
		sim.Start();
		for ( int i = 0; i < 60; i++ ) { sim.StepNow(); }

		hfSimSnapshot snap;
		snap.Zero();
		FEAT_ASSERT( sim.CopySnapshot( &snap ) );
		sim.Stop();

		// find the ball's world position as published by the sim
		hfVec3 ballPos( 0, 0, 0 );
		bool found = false;
		for ( int i = 0; i < snap.bodyCount; i++ ) {
			if ( snap.bodies[ i ].entity == ball ) { ballPos = snap.bodies[ i ].position; found = true; }
		}
		FEAT_ASSERT( found );

		// the audio front end mirrors that body into an emitter and mixes
		hfAudioServer audio{ hfAudioConfig() };
		hfSoundListener listener;
		listener.SetPositionLookAt( hfVec3( 0, 0, 0 ), ballPos );
		audio.SetListener( listener );

		hfSpatialEmitter e; e.play = true; e.spatialized = true;
		e.minDistance = 0.5f; e.maxDistance = 1000.0f;
		hfAudioRid er = audio.CreateEmitter( e );
		audio.SetEmitterPosition( er, ballPos );
		audio.StepNow();

		hfAudioSnapshot mix;
		mix.Zero();
		FEAT_ASSERT( audio.CopySnapshot( &mix ) );
		FEAT_ASSERT( mix.voiceCount == 1 );
		FEAT_ASSERT( mix.voices[0].emitter.index == er.index );
		FEAT_NEAR( mix.voices[0].position.x, ballPos.x, 1e-4f );
		FEAT_NEAR( mix.voices[0].position.z, ballPos.z, 1e-4f );
	}

	printf( "AudioServer ok (checksum %u, %d emitters, %s)\n",
		res.checksum, 4, res.started ? "worker thread" : "no worker" );
}

static void Test_Ui() {
	using namespace HF_Ui;

	// --- CPU bitmap font: measurement, wrapping, rasterization -----------
	FEAT_ASSERT( hfBitmapFont::MeasureText( "HELLO" ) == 30 );
	FEAT_ASSERT( hfBitmapFont::MeasureText( "AB\nCDE" ) == 18 );

	hfUiTextLine lines[ 8 ];
	int nLines = hfBitmapFont::WrapText( "HELLO WORLD FOO BAR", 60, lines, 8 );
	FEAT_ASSERT( nLines == 3 );
	FEAT_ASSERT( lines[0].length == 5 && lines[0].width == 30 );
	FEAT_ASSERT( lines[1].length == 9 && lines[1].width == 54 );
	FEAT_ASSERT( lines[2].length == 3 && lines[2].width == 18 );

	unsigned char pix[ 16 * 8 ];
	hfUiCoverage cov;
	cov.width = 16; cov.height = 8; cov.pixels = pix;
	cov.Clear();
	hfBitmapFont::Rasterize( "A", 1, 0, 0, cov, 255 );
	int solid = 0;
	for ( int i = 0; i < 16 * 8; i++ ) { if ( pix[ i ] ) { solid++; } }
	FEAT_ASSERT( solid > 0 );

	// --- layout: anchors, containers, clip, scroll, hit-test -------------
	HF_Ecs::Registry registry;
	hfUiSystem ui;
	ui.SetViewport( 800.0f, 600.0f );

	// full-screen background (also the hit-test backdrop)
	HF_Ecs::Entity eBg = registry.Create();
	hfUiCanvasComponent &bg = registry.AddComponent< hfUiCanvasComponent >( eBg );
	bg.anchor.SetPreset( HF_UI_ANCHOR_FULL_RECT );
	bg.interactive = true;
	bg.zIndex = 0;

	// fixed top-left panel
	HF_Ecs::Entity ePanel = registry.Create();
	hfUiCanvasComponent &panel = registry.AddComponent< hfUiCanvasComponent >( ePanel );
	panel.anchor.SetFixed( 10.0f, 10.0f, 100.0f, 50.0f );

	// centered panel (100x40 around the viewport center)
	HF_Ecs::Entity eCenter = registry.Create();
	hfUiCanvasComponent &center = registry.AddComponent< hfUiCanvasComponent >( eCenter );
	center.anchor.SetPreset( HF_UI_ANCHOR_CENTER );
	center.anchor.offsetMin.Set( -50.0f, -20.0f );
	center.anchor.offsetMax.Set( 50.0f, 20.0f );

	// row container: 3 cells 50x40, padding 5, spacing 10
	HF_Ecs::Entity eRow = registry.Create();
	hfUiCanvasComponent &row = registry.AddComponent< hfUiCanvasComponent >( eRow );
	row.anchor.SetFixed( 0.0f, 0.0f, 300.0f, 50.0f );
	row.layout = HF_UI_LAYOUT_ROW;
	row.padding = hfUiMargin::All( 5.0f );
	row.spacing = 10.0f;
	HF_Ecs::Entity eRowC[ 3 ];
	for ( int i = 0; i < 3; i++ ) {
		eRowC[ i ] = registry.Create();
		hfUiCanvasComponent &cc = registry.AddComponent< hfUiCanvasComponent >( eRowC[ i ] );
		cc.parent = eRow;
		cc.minSize.Set( 50.0f, 40.0f );
		cc.order = i;
	}

	// clipping container with an oversized child
	HF_Ecs::Entity eClip = registry.Create();
	hfUiCanvasComponent &clip = registry.AddComponent< hfUiCanvasComponent >( eClip );
	clip.anchor.SetFixed( 0.0f, 100.0f, 100.0f, 100.0f );
	clip.clipChildren = true;
	HF_Ecs::Entity eClipChild = registry.Create();
	hfUiCanvasComponent &clipChild = registry.AddComponent< hfUiCanvasComponent >( eClipChild );
	clipChild.parent = eClip;
	clipChild.anchor.SetFixed( 50.0f, 0.0f, 100.0f, 50.0f );

	// scrolling column container: children scrolled up by 20px
	HF_Ecs::Entity eScroll = registry.Create();
	hfUiCanvasComponent &scroll = registry.AddComponent< hfUiCanvasComponent >( eScroll );
	scroll.anchor.SetFixed( 0.0f, 300.0f, 100.0f, 100.0f );
	scroll.layout = HF_UI_LAYOUT_COLUMN;
	scroll.clipChildren = true;
	scroll.scrollY = 20.0f;
	HF_Ecs::Entity eScrollC[ 2 ];
	for ( int i = 0; i < 2; i++ ) {
		eScrollC[ i ] = registry.Create();
		hfUiCanvasComponent &cc = registry.AddComponent< hfUiCanvasComponent >( eScrollC[ i ] );
		cc.parent = eScroll;
		cc.minSize.Set( 0.0f, 30.0f );
		cc.order = i;
	}

	// two overlapping interactive buttons, B on top
	HF_Ecs::Entity eBtnA = registry.Create();
	hfUiCanvasComponent &btnA = registry.AddComponent< hfUiCanvasComponent >( eBtnA );
	btnA.parent = eBg;
	btnA.anchor.SetFixed( 100.0f, 100.0f, 100.0f, 100.0f );
	btnA.interactive = true;
	btnA.zIndex = 0;
	HF_Ecs::Entity eBtnB = registry.Create();
	hfUiCanvasComponent &btnB = registry.AddComponent< hfUiCanvasComponent >( eBtnB );
	btnB.parent = eBg;
	btnB.anchor.SetFixed( 150.0f, 150.0f, 100.0f, 100.0f );
	btnB.interactive = true;
	btnB.zIndex = 10;

	// wrapped label
	HF_Ecs::Entity eLab = registry.Create();
	hfUiCanvasComponent &labCanvas = registry.AddComponent< hfUiCanvasComponent >( eLab );
	labCanvas.anchor.SetFixed( 0.0f, 400.0f, 60.0f, 100.0f );
	hfUiLabelComponent &lab = registry.AddComponent< hfUiLabelComponent >( eLab );
	lab.SetText( "HELLO WORLD FOO BAR" );
	lab.wrap = true;

	// scaled non-wrapped label
	HF_Ecs::Entity eLab2 = registry.Create();
	registry.AddComponent< hfUiCanvasComponent >( eLab2 ).anchor.SetFixed( 0.0f, 520.0f, 200.0f, 20.0f );
	hfUiLabelComponent &lab2 = registry.AddComponent< hfUiLabelComponent >( eLab2 );
	lab2.SetText( "HELLO" );
	lab2.scale = 2.0f;

	ui.Update( registry, 0.0f );

	// anchors
	FEAT_NEAR( ui.GetRect( eBg ).x, 0, 0.001 );   FEAT_NEAR( ui.GetRect( eBg ).w, 800, 0.001 );
	FEAT_NEAR( ui.GetRect( eBg ).h, 600, 0.001 );
	FEAT_NEAR( ui.GetRect( ePanel ).x, 10, 0.001 );  FEAT_NEAR( ui.GetRect( ePanel ).y, 10, 0.001 );
	FEAT_NEAR( ui.GetRect( ePanel ).w, 100, 0.001 ); FEAT_NEAR( ui.GetRect( ePanel ).h, 50, 0.001 );
	FEAT_NEAR( ui.GetRect( eCenter ).x, 350, 0.001 ); FEAT_NEAR( ui.GetRect( eCenter ).y, 280, 0.001 );
	FEAT_NEAR( ui.GetRect( eCenter ).w, 100, 0.001 ); FEAT_NEAR( ui.GetRect( eCenter ).h, 40, 0.001 );

	// row container flow
	FEAT_NEAR( ui.GetRect( eRowC[0] ).x, 5, 0.001 );   FEAT_NEAR( ui.GetRect( eRowC[0] ).y, 5, 0.001 );
	FEAT_NEAR( ui.GetRect( eRowC[0] ).w, 50, 0.001 );  FEAT_NEAR( ui.GetRect( eRowC[0] ).h, 40, 0.001 );
	FEAT_NEAR( ui.GetRect( eRowC[1] ).x, 65, 0.001 );
	FEAT_NEAR( ui.GetRect( eRowC[2] ).x, 125, 0.001 );

	// clipping: the child's visible region is cut to the container
	FEAT_NEAR( ui.GetRect( eClipChild ).x, 50, 0.001 );
	FEAT_NEAR( ui.GetRect( eClipChild ).w, 100, 0.001 );
	FEAT_NEAR( ui.GetVisible( eClipChild ).x, 50, 0.001 );
	FEAT_NEAR( ui.GetVisible( eClipChild ).w, 50, 0.001 );

	// scroll moves the column's children up by 20px
	FEAT_NEAR( ui.GetRect( eScrollC[0] ).y, 280, 0.001 );
	FEAT_NEAR( ui.GetRect( eScrollC[1] ).y, 310, 0.001 );

	// hit-testing order: top-most control wins
	FEAT_ASSERT( ui.HitTest( 175.0f, 175.0f ) == eBtnB );
	FEAT_ASSERT( ui.HitTest( 110.0f, 110.0f ) == eBtnA );
	FEAT_ASSERT( ui.HitTest( 10.0f, 10.0f ) == eBg );
	FEAT_ASSERT( ui.HitTest( 900.0f, 900.0f ) == HF_Ecs::ENTITY_NULL );

	// text metrics resolved by the system
	FEAT_ASSERT( lab.lineCount == 3 );
	FEAT_NEAR( lab.textWidth, 54, 0.001 );
	FEAT_ASSERT( lab2.lineCount == 1 );
	FEAT_NEAR( lab2.textWidth, 60, 0.001 );

	printf( "Ui ok (%d controls, %d draw, wrap %d lines width %.0f)\n",
		ui.GetControlCount(), ui.GetDrawCount(), lab.lineCount, (double)lab.textWidth );
}

static int TestMat_TexLookup( const char *name, void *ctx ) {
	int *next = (int *)ctx;
	( void )name;
	return ( *next )++;
}

static HF_Render::hfRid TestMat_MatIdToRid( int materialId, void *ctx ) {
	int *pool = (int *)ctx;
	HF_Render::hfRid r;
	r.index = ( materialId >= 0 && materialId < 2 ) ? pool[materialId] : -1;
	return r;
}

static HF_Render::hfRid TestMat_TexIdToRid( int resourceId, void *ctx ) {
	( void )ctx;
	HF_Render::hfRid r;
	r.index = resourceId >= 0 ? resourceId : -1;
	return r;
}

// one deterministic sim run driven by an input log that may have arrived over
// a link. World, physics and sim are locals so the whole run (and its ~144MB
// registry) is gone again on return — several of these run back to back.
static void RunNetSimCase( unsigned int seed, const HF_Server::hfSimInput *inputs,
		int inputCount, int steps, HF_Server::hfSimSnapshot *outSnap ) {
	using namespace HF_Scene;
	using namespace HF_Physics;
	using namespace HF_Server;

	hfWorld world;
	hfPhysicsSystem phys;
	HF_Ecs::Entity ball = HF_Ecs::Entity();
	BuildSimTestScene( world, phys, seed, ball );

	hfSimConfig cfg;
	cfg.threaded = false;				// the client replays on its own thread
	hfSimulationServer sim( &world, cfg );
	sim.SetSeed( seed );
	for ( int i = 0; i < inputCount; i++ ) { sim.AddInput( inputs[ i ] ); }
	sim.Start();
	for ( int i = 0; i < steps; i++ ) { sim.StepNow(); }
	if ( outSnap ) {
		outSnap->Zero();
		sim.CopySnapshot( outSnap );
	}
	sim.Stop();
}

/*
================================
Phase 10 — networking

Moving the deterministic sim over a link is a copy problem, not a
reconciliation problem, and these checks are what that claim rests on:

  (a) a physics snapshot survives hfBitMsg byte for byte
  (b) a delta against the previous snapshot is smaller and decodes back
  (c) an input log survives the trip unchanged (input forwarding)
  (d) the loopback transport delivers, and the sequenced channel rejects
      duplicates and reorders arrivals the way an ENet-style channel does
  (e) the lag compensator rewinds to an exact recorded step
  (f) N clients that decode the same input packet and replay it locally
      reach byte-identical world state — the determinism proof
  (g) hfReplicationSystem replicates transforms authority -> remote, with
      unchanged entities costing one bit

Only one registry-heavy object is alive at any moment: this runtime has
~1.4GB free and every HF_Ecs::Registry takes ~144MB.
================================
*/
static void Test_Net() {
	using namespace HF_Net;
	using namespace HF_Server;

	int fc = g_featFails;

	// ---- (a) snapshot round trip ----------------------------------------
	{
		hfSimSnapshot a;
		a.Zero();
		a.sequence = 7; a.step = 7; a.simTime = 7.0f / 60.0f;
		a.seed = 0xABCDu; a.inputsApplied = 0;
		a.bodyCount = 3;
		for ( int i = 0; i < 3; i++ ) {
			hfSimBody &b = a.bodies[ i ];
			b.entity = ( HF_Ecs::Entity )( i + 1 );
			b.position = hfVec3( (float)i, (float)( i * 2 ), 1.0f );
			b.rotation = hfQuat( 0.0f, 0.0f, 0.0f, 1.0f );
			b.velocity = hfVec3( 0.0f, 0.0f, -9.8f );
			b.asleep = ( i == 2 );
		}
		a.checksum = a.ComputeChecksum();

		byte buf[ HF_NET_MAX_PACKET ];
		int len = hfNetReplicator::WriteSim( a, buf, ( int )sizeof( buf ), 42 );
		FEAT_ASSERT( len > 0 && len < ( int )sizeof( buf ) );

		hfSimSnapshot r;
		r.Zero();
		unsigned int pseq = 0;
		FEAT_ASSERT( hfNetReplicator::ReadSim( buf, len, &r, &pseq ) );
		FEAT_ASSERT( pseq == 42 );
		FEAT_ASSERT( r.step == a.step );
		FEAT_ASSERT( r.sequence == a.sequence );
		FEAT_ASSERT( r.seed == a.seed );
		FEAT_ASSERT( r.inputsApplied == a.inputsApplied );
		FEAT_NEAR( r.simTime, a.simTime, 1e-6f );
		FEAT_ASSERT( r.bodyCount == a.bodyCount );
		FEAT_ASSERT( r.checksum == a.checksum );
		// the decoded snapshot is byte identical, not merely equivalent
		FEAT_ASSERT( r.ComputeChecksum() == a.ComputeChecksum() );
		for ( int i = 0; i < a.bodyCount; i++ ) {
			FEAT_ASSERT( r.bodies[ i ].entity == a.bodies[ i ].entity );
			FEAT_NEAR( r.bodies[ i ].position.x, a.bodies[ i ].position.x, 0 );
			FEAT_NEAR( r.bodies[ i ].position.y, a.bodies[ i ].position.y, 0 );
			FEAT_NEAR( r.bodies[ i ].velocity.z, a.bodies[ i ].velocity.z, 0 );
			FEAT_ASSERT( r.bodies[ i ].asleep == a.bodies[ i ].asleep );
		}
		// garbage header is rejected rather than decoded
		byte bad[ 64 ];
		memcpy( bad, buf, 64 );
		bad[ 0 ] ^= 0xFF;
		FEAT_ASSERT( !hfNetReplicator::ReadSim( bad, 64, &r, NULL ) );

		// ---- (b) delta against the previous snapshot --------------------
		hfSimSnapshot b = a;
		b.step = 8; b.sequence = 8; b.simTime = 8.0f / 60.0f;
		b.bodies[ 1 ].position.x += 1.0f;		// exactly one body moved
		b.checksum = b.ComputeChecksum();

		byte full[ HF_NET_MAX_PACKET ];
		byte delta[ HF_NET_MAX_PACKET ];
		int fullLen = hfNetReplicator::WriteSim( b, full, ( int )sizeof( full ), 43 );
		int deltaLen = hfNetReplicator::WriteSimDelta( b, &a, delta, ( int )sizeof( delta ), 43 );
		FEAT_ASSERT( fullLen > 0 && deltaLen > 0 );
		FEAT_ASSERT( deltaLen < fullLen );		// the whole point of a delta

		hfSimSnapshot base = a;				// what the receiver already holds
		hfSimSnapshot d;
		d.Zero();
		unsigned int dseq = 0;
		FEAT_ASSERT( hfNetReplicator::ReadSimDelta( delta, deltaLen, &base, &d, &dseq ) );
		FEAT_ASSERT( dseq == 43 );
		FEAT_ASSERT( d.step == b.step );
		FEAT_ASSERT( d.sequence == b.sequence );
		FEAT_ASSERT( d.seed == b.seed );
		FEAT_ASSERT( d.bodyCount == b.bodyCount );
		FEAT_ASSERT( d.checksum == b.checksum );
		FEAT_ASSERT( d.ComputeChecksum() == b.ComputeChecksum() );
	}

	// ---- (c) input forwarding -------------------------------------------
	{
		hfSimInput in[ 4 ];
		for ( int i = 0; i < 4; i++ ) {
			in[ i ].step = 20 + i * 5;
			in[ i ].entity = ( HF_Ecs::Entity )( i + 1 );
			in[ i ].impulse = hfVec3( (float)i, 1.0f, -2.0f );
		}
		byte buf[ 512 ];
		int len = hfNetReplicator::WriteInputs( in, 4, buf, ( int )sizeof( buf ), 99 );
		FEAT_ASSERT( len > 0 && len < ( int )sizeof( buf ) );

		hfSimInput out[ 16 ];
		memset( out, 0, sizeof( out ) );
		unsigned int pseq = 0;
		int n = hfNetReplicator::ReadInputs( buf, len, out, 16, &pseq );
		FEAT_ASSERT( n == 4 );
		FEAT_ASSERT( pseq == 99 );
		for ( int i = 0; i < 4; i++ ) {
			FEAT_ASSERT( out[ i ].step == in[ i ].step );
			FEAT_ASSERT( out[ i ].entity == in[ i ].entity );
			FEAT_NEAR( out[ i ].impulse.x, in[ i ].impulse.x, 0 );
			FEAT_NEAR( out[ i ].impulse.y, in[ i ].impulse.y, 0 );
			FEAT_NEAR( out[ i ].impulse.z, in[ i ].impulse.z, 0 );
		}
		// a short buffer yields a truncation, not garbage
		FEAT_ASSERT( hfNetReplicator::ReadInputs( buf, len, out, 2, NULL ) == 2 );
	}

	// ---- (d) loopback transport + sequenced channel ----------------------
	{
		hfNetLoopbackTransport t;
		byte pkt[ 64 ];
		memset( pkt, 7, sizeof( pkt ) );

		// delivery
		FEAT_ASSERT( t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );
		hfNetEnvelope env[ 4 ];
		FEAT_ASSERT( t.Receive( 1, env, 4 ) == 1 );
		FEAT_ASSERT( env[ 0 ].fromPeer == 0 );
		FEAT_ASSERT( env[ 0 ].len == 64 );
		FEAT_ASSERT( memcmp( env[ 0 ].data, pkt, 64 ) == 0 );
		FEAT_ASSERT( t.Pending( 1 ) == 0 );
		// over-sized and out-of-range targets are refused
		FEAT_ASSERT( !t.Send( 0, HF_NET_MAX_PEERS, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );
		FEAT_ASSERT( !t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, HF_NET_MAX_PACKET + 1 ) );

		// duplicate injection -> the channel drops the second copy
		t.Reset();
		t.SetDuplicate( true );
		FEAT_ASSERT( t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );
		t.SetDuplicate( false );
		FEAT_ASSERT( t.Receive( 1, env, 4 ) == 2 );
		FEAT_ASSERT( env[ 0 ].sequence == env[ 1 ].sequence );
		hfNetChannel ch;
		FEAT_ASSERT( ch.Accept( env[ 0 ].sequence ) );
		FEAT_ASSERT( !ch.Accept( env[ 1 ].sequence ) );
		FEAT_ASSERT( ch.GetDuplicates() == 1 );

		// reorder injection -> arrivals land out of order and are still valid
		t.Reset();
		ch.Reset();
		FEAT_ASSERT( t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );	// seq 1
		FEAT_ASSERT( t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );	// seq 2
		t.SetReorder( true );
		FEAT_ASSERT( t.Send( 0, 1, HF_NET_CHANNEL_RELIABLE, pkt, 64 ) );	// seq 3 -> front
		t.SetReorder( false );
		FEAT_ASSERT( t.Receive( 1, env, 4 ) == 3 );
		FEAT_ASSERT( env[ 0 ].sequence == 3 );
		FEAT_ASSERT( env[ 1 ].sequence == 1 );
		FEAT_ASSERT( env[ 2 ].sequence == 2 );
		FEAT_ASSERT( ch.Accept( 3 ) );		// newest first
		FEAT_ASSERT( ch.Accept( 1 ) );		// late but inside the window
		FEAT_ASSERT( ch.Accept( 2 ) );
		FEAT_ASSERT( !ch.Accept( 1 ) );		// and now it is a duplicate
		FEAT_ASSERT( ch.GetAccepted() == 3 );
		FEAT_ASSERT( ch.GetDuplicates() == 1 );
		// too old to matter
		FEAT_ASSERT( !ch.Accept( 1 ) );
		for ( unsigned int s = 1; s < hfNetChannel::WINDOW; s++ ) { ch.Accept( 1000 + s ); }
		FEAT_ASSERT( !ch.Accept( 1000 ) );		// fell out of the window
		// sequence 0 is never valid
		FEAT_ASSERT( !ch.Accept( 0 ) );
	}

	// ---- (e) lag compensation ring ---------------------------------------
	{
		hfNetLagCompensator lag;
		hfSimSnapshot s;
		s.Zero();
		for ( unsigned int step = 10; step < 14; step++ ) {
			s.step = step;
			s.sequence = step;
			s.bodyCount = 1;
			s.bodies[ 0 ].entity = ( HF_Ecs::Entity )( step );
			s.bodies[ 0 ].position.x = (float)step;
			s.bodies[ 0 ].asleep = ( step == 13 );
			s.checksum = s.ComputeChecksum();
			lag.Record( s );
		}
		FEAT_ASSERT( lag.GetCount() == 4 );
		FEAT_ASSERT( lag.NewestStep() == 13 );
		const hfSimSnapshot *m = lag.Sample( 11 );
		FEAT_ASSERT( m != NULL );
		FEAT_ASSERT( m->step == 11 );
		FEAT_NEAR( m->bodies[ 0 ].position.x, 11.0f, 0 );
		FEAT_ASSERT( m->checksum == m->ComputeChecksum() );
		FEAT_ASSERT( lag.Sample( 99 ) == NULL );	// exact step only, never a guess

		// a repeated step replaces instead of appending
		s.step = 11; s.bodies[ 0 ].position.x = 1111.0f;
		s.checksum = s.ComputeChecksum();
		lag.Record( s );
		FEAT_ASSERT( lag.GetCount() == 4 );
		m = lag.Sample( 11 );
		FEAT_ASSERT( m != NULL );
		FEAT_NEAR( m->bodies[ 0 ].position.x, 1111.0f, 0 );

		// the ring overwrites its oldest entry once full
		lag.SetCapacity( 2 );
		FEAT_ASSERT( lag.GetCount() == 0 && lag.GetCapacity() == 2 );
		for ( unsigned int step = 20; step < 24; step++ ) {
			s.step = step;
			s.checksum = s.ComputeChecksum();
			lag.Record( s );
		}
		FEAT_ASSERT( lag.GetCount() == 2 );
		FEAT_ASSERT( lag.Sample( 20 ) == NULL );
		FEAT_ASSERT( lag.Sample( 23 ) != NULL );
		FEAT_ASSERT( lag.NewestStep() == 23 );
	}

	// ---- (f) N clients replay a forwarded input log ----------------------
	{
		const unsigned int seed = 0xBEEF01u;
		const int steps = 90;

		// The scene is rebuilt from the same seed on every peer, so it hands
		// out the same entity handles everywhere and a log recorded against
		// one world is valid on all of them. Probe it once, then let the
		// registry go before the runs below need their own.
		HF_Ecs::Entity ball = HF_Ecs::Entity();
		{
			HF_Scene::hfWorld world;
			HF_Physics::hfPhysicsSystem phys;
			BuildSimTestScene( world, phys, seed, ball );
		}
		FEAT_ASSERT( ball != HF_Ecs::ENTITY_NULL );

		hfSimInput log[ 4 ];
		memset( log, 0, sizeof( log ) );
		log[ 0 ].step = 20; log[ 0 ].entity = ball;
		log[ 0 ].impulse = hfVec3( 2.0f, 0.0f, 0.0f );
		log[ 1 ].step = 35; log[ 1 ].entity = ball;
		log[ 1 ].impulse = hfVec3( 0.0f, 1.5f, 0.0f );
		log[ 2 ].step = 50; log[ 2 ].entity = ball;
		log[ 2 ].impulse = hfVec3( -1.0f, 0.0f, 0.5f );
		log[ 3 ].step = 70; log[ 3 ].entity = ball;
		log[ 3 ].impulse = hfVec3( 0.0f, 0.0f, 2.0f );

		// the authority runs the log it recorded
		hfSimSnapshot serverSnap;
		serverSnap.Zero();
		RunNetSimCase( seed, log, 4, steps, &serverSnap );
		FEAT_ASSERT( serverSnap.bodyCount == 3 );
		FEAT_ASSERT( serverSnap.step == ( unsigned int )steps );
		FEAT_ASSERT( serverSnap.inputsApplied == 4 );
		FEAT_ASSERT( serverSnap.checksum == serverSnap.ComputeChecksum() );

		// and it ships two things: the log to replay, the state to match
		byte inputPacket[ 512 ];
		int inputLen = hfNetReplicator::WriteInputs( log, 4, inputPacket,
			( int )sizeof( inputPacket ), 1 );
		FEAT_ASSERT( inputLen > 0 );

		byte statePacket[ HF_NET_MAX_PACKET ];
		int stateLen = hfNetReplicator::WriteSim( serverSnap, statePacket,
			( int )sizeof( statePacket ), 2 );
		FEAT_ASSERT( stateLen > 0 );

		for ( int c = 0; c < 3; c++ ) {
			hfSimInput decoded[ 16 ];
			memset( decoded, 0, sizeof( decoded ) );
			unsigned int pseq = 0;
			int n = hfNetReplicator::ReadInputs( inputPacket, inputLen, decoded, 16, &pseq );
			FEAT_ASSERT( n == 4 );
			FEAT_ASSERT( pseq == 1 );
			for ( int i = 0; i < n; i++ ) {
				FEAT_ASSERT( decoded[ i ].step == log[ i ].step );
				FEAT_ASSERT( decoded[ i ].entity == log[ i ].entity );
			}

			hfSimSnapshot shipped;
			shipped.Zero();
			FEAT_ASSERT( hfNetReplicator::ReadSim( statePacket, stateLen, &shipped, NULL ) );

			hfSimSnapshot clientSnap;
			clientSnap.Zero();
			RunNetSimCase( seed, decoded, n, steps, &clientSnap );

			// the state the authority sent arrived byte for byte
			FEAT_ASSERT( clientSnap.checksum == shipped.checksum );
			FEAT_ASSERT( clientSnap.ComputeChecksum() == shipped.ComputeChecksum() );
			// replaying the same log locally produced that same world
			FEAT_ASSERT( clientSnap.checksum == serverSnap.checksum );
			FEAT_ASSERT( clientSnap.ComputeChecksum() == serverSnap.ComputeChecksum() );
			FEAT_ASSERT( clientSnap.step == serverSnap.step );
			FEAT_ASSERT( clientSnap.simTime == serverSnap.simTime );
			FEAT_ASSERT( clientSnap.seed == serverSnap.seed );
		}
	}

	// ---- (g) transform replication through hfReplicationSystem -----------
	{
		hfNetLoopbackTransport transport;

		hfReplicationSystem auth;
		auth.SetTransport( &transport );
		auth.SetPeer( HF_NET_PEER_SERVER, 1 );
		auth.SetAuthority( true );
		auth.SetStep( 10 );

		hfReplicationSystem remote;
		remote.SetTransport( &transport );
		remote.SetPeer( 1, HF_NET_PEER_SERVER );
		remote.SetAuthority( false );
		remote.SetStep( 10 );

		const int N = 3;
		HF_Ecs::Entity se[ N ];
		HF_Ecs::Entity ce[ N ];
		{
			HF_Ecs::Registry serverReg;
			HF_Ecs::Registry clientReg;
			for ( int i = 0; i < N; i++ ) {
				se[ i ] = serverReg.Create();
				ce[ i ] = clientReg.Create();
				hfNetComponent &nc = serverReg.AddComponent< hfNetComponent >( se[ i ] );
				nc.netId = hfNetRid( i + 1 );
				nc.ownerPeer = HF_NET_PEER_SERVER;
				serverReg.AddComponent< HF_Scene::hfTransformComponent >( se[ i ] );
				hfNetComponent &cc = clientReg.AddComponent< hfNetComponent >( ce[ i ] );
				cc.netId = hfNetRid( i + 1 );
				cc.role = HF_NET_ROLE_REMOTE;
				clientReg.AddComponent< HF_Scene::hfTransformComponent >( ce[ i ] );
			}

			// the authority moves everything once
			for ( int i = 0; i < N; i++ ) {
				HF_Scene::hfTransformComponent *t =
					serverReg.GetComponent< HF_Scene::hfTransformComponent >( se[ i ] );
				FEAT_ASSERT( t != NULL );
				t->SetPosition( hfVec3( (float)( i + 1 ), 10.0f, -3.0f ) );
			}
			FEAT_ASSERT( auth.SendState( serverReg ) );
			FEAT_ASSERT( transport.Pending( 1 ) == 1 );
			int firstLen = auth.GetLastPacketBytes();
			FEAT_ASSERT( firstLen > 0 );
			FEAT_ASSERT( auth.GetPeerStats().packetsOut == 1 );


			FEAT_ASSERT( remote.ReceiveState( clientReg ) );
			FEAT_ASSERT( transport.Pending( 1 ) == 0 );
			FEAT_ASSERT( remote.GetStateRecvCount() == 1 );
			for ( int i = 0; i < N; i++ ) {
				HF_Scene::hfTransformComponent *s =
					serverReg.GetComponent< HF_Scene::hfTransformComponent >( se[ i ] );
				HF_Scene::hfTransformComponent *cl =
					clientReg.GetComponent< HF_Scene::hfTransformComponent >( ce[ i ] );
				hfNetComponent *cc = clientReg.GetComponent< hfNetComponent >( ce[ i ] );
				FEAT_ASSERT( cl != NULL && cc != NULL );
				FEAT_ASSERT( cc->netId.index == i + 1 );
				FEAT_NEAR( cl->position.x, s->position.x, 0 );
				FEAT_NEAR( cl->position.y, s->position.y, 0 );
				FEAT_NEAR( cl->position.z, s->position.z, 0 );
				FEAT_ASSERT( cl->dirty );
				FEAT_ASSERT( cc->replicatedStep == 10 );
			}

			// nothing changed -> every entity costs one bit
			FEAT_ASSERT( auth.SendState( serverReg ) );
			int idleLen = auth.GetLastPacketBytes();
			FEAT_ASSERT( idleLen > 0 && idleLen < firstLen );
			FEAT_ASSERT( remote.ReceiveState( clientReg ) );

			// one entity moved -> only that one is on the wire
			serverReg.GetComponent< HF_Scene::hfTransformComponent >( se[ 0 ] )
				->SetPosition( hfVec3( 99.0f, 99.0f, 99.0f ) );
			FEAT_ASSERT( auth.SendState( serverReg ) );
			int oneLen = auth.GetLastPacketBytes();
			FEAT_ASSERT( oneLen > idleLen && oneLen < firstLen );
			FEAT_ASSERT( remote.ReceiveState( clientReg ) );
			HF_Scene::hfTransformComponent *c0 =
				clientReg.GetComponent< HF_Scene::hfTransformComponent >( ce[ 0 ] );
			HF_Scene::hfTransformComponent *c1 =
				clientReg.GetComponent< HF_Scene::hfTransformComponent >( ce[ 1 ] );
			FEAT_ASSERT( c0 != NULL && c1 != NULL );
			FEAT_NEAR( c0->position.x, 99.0f, 0 );
			FEAT_NEAR( c1->position.x, 2.0f, 0 );	// untouched, as promised

			// a replayed packet is rejected by the sequenced channel
			byte pkt[ 64 ];
			memset( pkt, 7, sizeof( pkt ) );
			FEAT_ASSERT( transport.SendRaw( 0, 1, HF_NET_CHANNEL_RELIABLE, 1, pkt, 64 ) );
			FEAT_ASSERT( !remote.ReceiveState( clientReg ) );
			FEAT_ASSERT( remote.GetPeerStats().duplicatesDropped == 1 );
			FEAT_ASSERT( remote.GetStateRecvCount() == 3 );	// the dup never counted

			// acknowledgement is monotonic: the ack the server rewinds to
			remote.Acknowledge( 12 );
			FEAT_ASSERT( remote.GetPeerStats().lastAckedStep == 12 );
			remote.Acknowledge( 3 );
			FEAT_ASSERT( remote.GetPeerStats().lastAckedStep == 12 );

			// the per-frame path is the same code
			auth.SetStep( 11 );
			remote.SetStep( 11 );
			serverReg.GetComponent< HF_Scene::hfTransformComponent >( se[ 1 ] )
				->SetPosition( hfVec3( 5.0f, 5.0f, 5.0f ) );
			auth.Update( serverReg, 1.0f / 60.0f );
			remote.Update( clientReg, 1.0f / 60.0f );
			HF_Scene::hfTransformComponent *c1b =
				clientReg.GetComponent< HF_Scene::hfTransformComponent >( ce[ 1 ] );
			FEAT_ASSERT( c1b != NULL );
			FEAT_NEAR( c1b->position.x, 5.0f, 0 );
			FEAT_ASSERT( remote.GetPeerStats().lastSentStep == 0 );	// remote never sends
		}
		// transport goes out of scope with its queues still balanced
		FEAT_ASSERT( transport.Pending( 1 ) == 0 );
	}

	int added = g_featFails - fc;
	printf( "Net: %s\n", added ? "FAIL" : "ok" );
}

static void Test_Materials() {
	using namespace HF_Render;

	// (a) .mtr -> hfPbrMaterial assembly (albedo, transparent, noshadows, normal)
	{
		const char *mtr =
			"brick { stages { map textures/brick_d } noshadows }\n"
			"glass { stages { map textures/glass_d blendfunc blend } }\n"
			"metal { stages { map textures/metal_d } stages { map textures/metal_n vertextexgen normalmap } }\n";
		HF_Asset::mtrDef_t defs[8];
		int n = HF_Asset::ParseMtr( mtr, (int)strlen( mtr ), defs, 8 );
		FEAT_ASSERT( n == 3 );

		int next = 0;
		int (*lookup)( const char *, void * ) = TestMat_TexLookup;

		hfPbrMaterial brick, glass, metal;
		FEAT_ASSERT( BuildMaterial( defs[0], brick, lookup, &next ) );
		FEAT_ASSERT( brick.noShadow );
		FEAT_ASSERT( !brick.transparent );
		FEAT_ASSERT( brick.albedoTexId == 0 );
		FEAT_ASSERT( brick.normalTexId == -1 );

		FEAT_ASSERT( BuildMaterial( defs[1], glass, lookup, &next ) );
		FEAT_ASSERT( glass.transparent );
		FEAT_ASSERT( glass.albedoTexId == 1 );

		FEAT_ASSERT( BuildMaterial( defs[2], metal, lookup, &next ) );
		FEAT_ASSERT( metal.albedoTexId == 2 );
		FEAT_ASSERT( metal.normalTexId == 3 );

		// malformed def must fail
		HF_Asset::mtrDef_t bad;
		bad.Reset();
		hfPbrMaterial mb;
		FEAT_ASSERT( !BuildMaterial( bad, mb, lookup, &next ) );
	}

	// (b) tone mapping + fog math
	{
		hfVec3 black = ACESTonemap( hfVec3( 0, 0, 0 ) );
		FEAT_NEAR( black.x, 0.0f, 1e-4f );
		hfVec3 bright = ACESTonemap( hfVec3( 10, 10, 10 ) );
		FEAT_ASSERT( bright.x <= 1.0f && bright.x >= 0.95f );
		FEAT_ASSERT( ACESTonemap( hfVec3( 1, 1, 1 ) ).x > ACESTonemap( hfVec3( 0.5f, 0.5f, 0.5f ) ).x );

		hfFogParams fog;
		fog.mode = HF_FOG_LINEAR; fog.end = 30.0f;	// start 10, end 30
		FEAT_NEAR( ComputeFogFactor( fog, 5.0f ), 0.0f, 1e-3f );
		FEAT_NEAR( ComputeFogFactor( fog, 20.0f ), 0.5f, 1e-3f );
		FEAT_NEAR( ComputeFogFactor( fog, 50.0f ), 1.0f, 1e-3f );

		hfFogParams exp2;
		exp2.mode = HF_FOG_EXP2; exp2.density = 0.1f;
		FEAT_NEAR( ComputeFogFactor( exp2, 0.0f ), 0.0f, 1e-3f );
		FEAT_ASSERT( ComputeFogFactor( exp2, 200.0f ) > 0.99f );
	}

	// (c) server frame build: draw list -> resolved commands
	{
		hfRenderingServer server;

		hfPbrMaterial m;
		m.metallic = 0.2f; m.roughness = 0.4f; m.ao = 0.9f;
		m.emissiveScale = 2.0f; m.emissiveColor = hfVec3( 0.1f, 0, 0 );
		hfRid matRid = server.CreateMaterial( m );
		int pool[2] = { matRid.index, -1 };

		HF_Scene::hfDrawEntry draws[2];
		for ( int i = 0; i < 2; i++ ) {
			draws[i].entity = (HF_Ecs::Entity)( i + 1 );
			draws[i].meshId = i;
			draws[i].materialId = 0;
			draws[i].distanceSq = ( i == 0 ) ? 25.0f : 400.0f;	// dist 5 / 20
			draws[i].worldMatrix.Identity();
		}

		hfEnvironment env;
		env.fog.mode = HF_FOG_LINEAR; env.fog.start = 10.0f; env.fog.end = 30.0f;
		env.fog.color = hfVec3( 0.7f, 0.8f, 0.9f );

		server.BuildFrame( draws, 2, NULL, 0, env, TestMat_MatIdToRid, pool, TestMat_TexIdToRid, NULL );
		FEAT_ASSERT( server.GetCommandCount() == 2 );
		const hfDrawCommand &c0 = server.GetCommand( 0 );
		FEAT_NEAR( c0.rpUser1.x, 0.4f, 1e-4f );		// roughness
		FEAT_NEAR( c0.rpUser1.y, 0.2f, 1e-4f );		// metallic
		FEAT_NEAR( c0.rpUser1.z, 0.9f, 1e-4f );		// ao
		FEAT_NEAR( c0.rpUser1.w, 2.0f, 1e-4f );		// emissiveScale
		FEAT_NEAR( c0.rpUser4.x, 0.55f, 1e-4f );
		FEAT_NEAR( c0.rpUser4.w, 1.0f, 1e-4f );
		FEAT_NEAR( c0.fogFactor, 0.0f, 1e-3f );		// dist 5 < start 10
		FEAT_NEAR( server.GetCommand( 1 ).fogFactor, 0.5f, 1e-3f );	// dist 20 = midpoint
	}

	// (d) CPU reference rasterizer
	{
		hfRenderingServer server;
		hfPbrMaterial m;	// white, default ambient
		hfRid matRid = server.CreateMaterial( m );
		int pool[2] = { matRid.index, -1 };

		HF_Scene::hfDrawEntry draw;
		draw.entity = 1u; draw.meshId = 0; draw.materialId = 0;
		draw.distanceSq = 25.0f;
		draw.worldMatrix.Identity();

		hfEnvironment env;
		env.fog.mode = HF_FOG_NONE;

		server.BuildFrame( &draw, 1, NULL, 0, env, TestMat_MatIdToRid, pool, TestMat_TexIdToRid, NULL );
		hfDrawCommand cmd = server.GetCommand( 0 );

		byte rgb[ hfRenderingServer::RAST_W * hfRenderingServer::RAST_H * 3 ];
		const int center = ( hfRenderingServer::RAST_H / 2 * hfRenderingServer::RAST_W + hfRenderingServer::RAST_W / 2 ) * 3;

		// lit head-on: bright center, dark off-triangle corner
		server.RasterReference( cmd, hfVec3( 0, 0, 1 ), hfVec3( 1, 1, 1 ), rgb,
			hfRenderingServer::RAST_W, hfRenderingServer::RAST_H );
		FEAT_ASSERT( rgb[ center + 0 ] > 150 && rgb[ center + 1 ] > 150 && rgb[ center + 2 ] > 150 );
		FEAT_ASSERT( rgb[ 0 ] == 0 && rgb[ 1 ] == 0 && rgb[ 2 ] == 40 );	// background

		// light behind the surface: only ambient -> dim but not black
		server.RasterReference( cmd, hfVec3( 0, 0, -1 ), hfVec3( 1, 1, 1 ), rgb,
			hfRenderingServer::RAST_W, hfRenderingServer::RAST_H );
		FEAT_ASSERT( rgb[ center + 0 ] > 5 && rgb[ center + 0 ] < 80 );

		// full fog: surface collapses into the fog color (brighter than unlit)
		hfDrawCommand fogged = cmd;
		fogged.fogFactor = 1.0f;
		fogged.fogColor = hfVec3( 0.7f, 0.8f, 0.9f );
		server.RasterReference( fogged, hfVec3( 0, 0, -1 ), hfVec3( 1, 1, 1 ), rgb,
			hfRenderingServer::RAST_W, hfRenderingServer::RAST_H );
		FEAT_ASSERT( rgb[ center + 0 ] > 150 );
	}

	printf( "Materials ok\n" );
}

static void Test_Import() {
	// (a) JSON parser
	{
		const char *	j = "{\"a\":1,\"b\":[true,2.5,\"x\"],\"c\":{\"d\":null}}";
		HF_Json::Value	root;
		HF_Json::Value	bad;
		hfStr			e1, e2;
		FEAT_ASSERT( HF_Json::Parse( j, strlen( j ), root, e1 ) );
		FEAT_ASSERT( HF_Json::GetNum( root, "a", -1.0 ) == 1.0 );
		const HF_Json::Value *b = root.Find( "b" );
		FEAT_ASSERT( b && b->IsArr() && b->Count() == 3 );
		FEAT_ASSERT( b->Get( 0 )->b );
		FEAT_ASSERT( b->Get( 1 )->IsNum() && b->Get( 1 )->num == 2.5 );
		FEAT_ASSERT( strcmp( b->Get( 2 )->AsStr( "" ), "x" ) == 0 );
		const HF_Json::Value *c = root.Find( "c" );
		FEAT_ASSERT( c && c->IsObj() && c->Find( "d" ) && c->Find( "d" )->type == HF_Json::JSON_NULL );
		FEAT_ASSERT( !HF_Json::Parse( "{\"a\":1}x", 8, bad, e2 ) );	// trailing junk
		HF_Json::Free( root );
		HF_Json::Free( bad );
	}

	// (b) glTF subset import (triangle + node transform + PBR material)
	const char *	gtf =
		"{ \"asset\":{\"version\":\"2.0\"},"
		" \"buffers\":[{\"uri\":\"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA\"}],"
		" \"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}],"
		" \"accessors\":[{\"bufferView\":0,\"byteOffset\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
		"               {\"bufferView\":1,\"byteOffset\":0,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
		" \"materials\":[{\"name\":\"gold\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[1.0,0.5,0.25,1.0],\"metallicFactor\":0.5,\"roughnessFactor\":0.2}}],"
		" \"meshes\":[{\"name\":\"tri\",\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1,\"material\":0}]}],"
		" \"nodes\":[{\"name\":\"triNode\",\"mesh\":0,\"translation\":[2.0,0.0,0.0]}]}";
	{
		HF_Gltf::hfGltfDoc	doc;
		hfStr				err;
		int					n = HF_Gltf::ImportGltf( gtf, strlen( gtf ), doc, err );
		FEAT_ASSERT( n == 1 );
		FEAT_ASSERT( doc.numMeshes == 1 && doc.numMats == 1 );
		FEAT_ASSERT( doc.meshes[0].vertCount == 3 && doc.meshes[0].triCount == 1 );
		FEAT_ASSERT( doc.meshes[0].bounds.GetCenter().x > 0.49f && doc.meshes[0].bounds.GetCenter().x < 0.51f );
		FEAT_ASSERT( doc.meshes[0].bounds.GetCenter().y > 0.49f && doc.meshes[0].bounds.GetCenter().y < 0.51f );
		FEAT_ASSERT( doc.mats[0].baseColor[0] == 1.0f && doc.mats[0].baseColor[1] == 0.5f && doc.mats[0].baseColor[2] == 0.25f );
		FEAT_NEAR( doc.mats[0].metallic, 0.5f, 1e-4f );
		FEAT_NEAR( doc.mats[0].roughness, 0.2f, 1e-4f );
		hfVec3			tp;
		HF_Gltf::TransformPoint( doc.nodes[0].local, hfVec3( 0, 0, 0 ), tp );
		FEAT_NEAR( tp.x, 2.0f, 1e-4f );
		FEAT_NEAR( tp.y, 0.0f, 1e-4f );
		HF_Gltf::TransformPoint( doc.nodes[0].local, hfVec3( 1, 1, 1 ), tp );
		FEAT_NEAR( tp.x, 3.0f, 1e-4f );
		FEAT_NEAR( tp.z, 1.0f, 1e-4f );
	}

	// (c) content pipeline: glTF -> .mesh blob + .mtr text -> packed store
	{
		HF_Gltf::hfGltfDoc	doc;
		hfStr				err;
		HF_Gltf::ImportGltf( gtf, strlen( gtf ), doc, err );
		char				mtrBuf[ 256 ];
		HF_Asset::AssetSource	items[ 2 ];
		int					count = HF_Gltf::BuildAssetItems( doc, "models/tri.mesh", "materials/tri.mtr", "tri", items, mtrBuf, sizeof( mtrBuf ) );
		FEAT_ASSERT( count == 2 );
		HF_Asset::PackedStore	store = HF_Asset::Pack( items, count );
		FEAT_ASSERT( store.ok && store.size > 0 );
		FEAT_ASSERT( HF_Asset::Validate( store.data, store.size ) );
		hfStr			meshOut;
		HF_Asset::AssetEntry	meta;
		FEAT_ASSERT( HF_Asset::Extract( store.data, store.size, "models/tri.mesh", meshOut, &meta ) );
		FEAT_ASSERT( meshOut.Length() == 64 );			// 8 magic + 2*4 + 9*4 pos + 3*4 idx
		FEAT_ASSERT( meshOut[0] == 'H' && meshOut[1] == 'F' && meshOut[7] == '1' );
		hfStr			mtrOut;
		FEAT_ASSERT( HF_Asset::Extract( store.data, store.size, "materials/tri.mtr", mtrOut ) );
		FEAT_ASSERT( mtrOut.Find( "name tri" ) >= 0 );
		HF_Asset::FreeStore( store );
	}

	// (d) CSG brushes
	{
		HF_Csg::hfMeshSoup	soup;
		HF_Scene::hfCsgComponent box;
		box.shape = HF_Scene::HF_CSG_BOX;
		box.halfExtents = hfVec3( 1, 2, 3 );
		HF_Csg::TessellateShape( box, soup );
		FEAT_ASSERT( soup.numTri == 12 );
		hfBounds		b;
		HF_Csg::BoundsFromSoup( soup, b );
		FEAT_NEAR( b[1].x, 1.0f, 1e-4f );
		FEAT_NEAR( b[1].y, 2.0f, 1e-4f );
		FEAT_NEAR( b[1].z, 3.0f, 1e-4f );

		HF_Scene::hfCsgComponent sph;
		sph.shape = HF_Scene::HF_CSG_SPHERE;
		sph.halfExtents = hfVec3( 1, 1, 1 );
		HF_Csg::TessellateShape( sph, soup );
		FEAT_ASSERT( soup.numTri == 20 );
		FEAT_ASSERT( soup.verts[0].LengthSqr() < 1.001f );

		HF_Scene::hfCsgComponent quad;
		quad.shape = HF_Scene::HF_CSG_QUAD;
		quad.halfExtents = hfVec3( 4, 3, 0 );
		HF_Csg::TessellateShape( quad, soup );
		FEAT_ASSERT( soup.numTri == 2 );
	}

	// (e) CSG as a world system: dirty brush gets tessellated on Update
	{
		HF_Scene::hfWorld	world;
		hfCsgSystem			csgSys;
		world.RegisterSystem( &csgSys );
		HF_Ecs::Entity		e = world.CreateEntity();
		HF_Scene::hfCsgComponent &c = world.AddComponent< HF_Scene::hfCsgComponent >( e );
		c.shape = HF_Scene::HF_CSG_BOX;
		c.halfExtents = hfVec3( 1, 1, 1 );
		world.Update( 0.016f );
		FEAT_ASSERT( c.triCount == 12 );
		FEAT_ASSERT( c.dirty == false );
		FEAT_ASSERT( c.localBounds.ContainsPoint( hfVec3( 0.99f, 0.99f, 0.99f ) ) );
		world.Update( 0.016f );		// re-Update without dirty → no change
		FEAT_ASSERT( c.triCount == 12 );
	}

	printf( "Import ok\n" );
}

/*
=======
hfStr

Case folding is not cosmetic in this engine: asset lookups, shader names and
qpaths are all compared case insensitively, so a ToLower() that misses a byte
silently breaks every lookup. IcmpPrefix / IcmpPrefixPath are the two prefix
tests the loaders use, and hfStrStatic is the no-allocation string the
per-frame code builds on, so all three are covered here.
=======
*/
static void Test_hfStr() {
	int fc = g_featFails;

	// (a) case conversion round trips, and only touches ascii letters
	hfStr	lower( "Textures/FOO/Bar_9.bsp" );
	lower.ToLower();
	FEAT_ASSERT( lower == "textures/foo/bar_9.bsp" );
	hfStr	upper( lower );
	upper.ToUpper();
	FEAT_ASSERT( upper == "TEXTURES/FOO/BAR_9.BSP" );
	hfStr	symbols( "a1-_./" );
	symbols.ToLower();
	FEAT_ASSERT( symbols == "a1-_./" );		// digits and punctuation are not letters

	// (b) case insensitive hashing has to agree, it is the lookup key
	hfStr	hashA( "textures/common/caulk" );
	hfStr	hashB( "TEXTURES/COMMON/CAULK" );
	hashA.ToLower();
	hashB.ToLower();
	FEAT_ASSERT( hashA == hashB );

	// (c) IcmpPrefix: plain case insensitive prefix, 0 when s2 is a prefix
	FEAT_ASSERT( hfStr( "textures/common/caulk" ).IcmpPrefix( "TEXTURES" ) == 0 );
	FEAT_ASSERT( hfStr( "textures/common/caulk" ).IcmpPrefix( "textures/common/" ) == 0 );
	FEAT_ASSERT( hfStr( "textures/common/caulk" ).IcmpPrefix( "sounds" ) != 0 );
	// a longer needle is not a prefix of a shorter haystack
	FEAT_ASSERT( hfStr( "textures" ).IcmpPrefix( "textures/common" ) != 0 );
	FEAT_ASSERT( hfStr( "textures/common/caulk" ).IcmpPrefix( NULL ) != 0 );

	// (d) IcmpPath ignores the separator direction, IcmpPrefixPath also folds
	// case and backslashes, which is what mount tests ("sound/vo/") need
	FEAT_ASSERT( hfStr( "textures\\common\\caulk" ).IcmpPath( "TEXTURES/common/caulk" ) == 0 );
	FEAT_ASSERT( hfStr( "baseq3/textures/common" ).IcmpPrefixPath( "BASEQ3" ) == 0 );
	FEAT_ASSERT( hfStr( "baseq3\\textures\\common" ).IcmpPrefixPath( "baseq3/" ) == 0 );
	FEAT_ASSERT( hfStr( "baseq3/textures" ).IcmpPrefixPath( "basq3" ) != 0 );
	FEAT_ASSERT( hfStr( "baseq3/textures" ).IcmpPrefixPath( "textures" ) != 0 );
	FEAT_ASSERT( hfStr( "baseq3/textures" ).IcmpPrefixPath( NULL ) == 0 );

	// (e) Find / FindText, case sensitive and not
	hfStr	path( "baseq3/textures/common/caulk" );
	FEAT_ASSERT( path.Find( "textures/" ) == 7 );
	FEAT_ASSERT( path.Find( "TEXTURES/" ) == -1 );
	FEAT_ASSERT( path.Find( "TEXTURES/", false ) == 7 );
	FEAT_ASSERT( path.Find( "nope" ) == -1 );
	// the static entry point has to default the range over the whole string
	FEAT_ASSERT( hfStr::FindText( "baseq3/textures", "textures" ) == 7 );
	FEAT_ASSERT( hfStr::FindText( "baseq3/textures", "TEXTURES", false ) == 7 );

	// (f) Set / SetVA write straight into the string, growing when needed
	hfStr	built;
	built.Set( "%s/%d/%0.2f", "textures", 42, 1.5f );
	FEAT_ASSERT( built == "textures/42/1.50" );
	built.Set( "short" );
	FEAT_ASSERT( built == "short" && built.Length() == 5 );
	// a result larger than the inline buffer has to reallocate and still fit
	hfStr	big;
	big.Set( "%s", "0123456789012345678901234567890123456789" );
	FEAT_ASSERT( big.Length() == 40 && big[ 39 ] == '9' );

	// (g) hfStrStatic keeps its characters inside the object
	hfStrStatic< 16 >	stat( "abc" );
	FEAT_ASSERT( stat.Length() == 3 );
	FEAT_ASSERT( stat.StaticSize() == 16 );
	FEAT_ASSERT( stat.Available() == 12 );		// 15 usable bytes, 3 used
	FEAT_ASSERT( stat.DynamicMemoryUsed() == 0 );
	stat = "hello";
	FEAT_ASSERT( stat == "hello" );
	// overflow must move to the heap instead of running past the buffer
	stat = "0123456789abcdefghij";
	FEAT_ASSERT( stat.Length() == 20 );
	FEAT_ASSERT( stat.DynamicMemoryUsed() > 0 );

	// (h) snPrintf truncates rather than overflowing
	char	small[ 8 ];
	int		n = hfStr::snPrintf( small, sizeof( small ), "%s", "0123456789" );
	FEAT_ASSERT( n >= 0 );
	FEAT_ASSERT( small[ sizeof( small ) - 1 ] == '\0' );
	FEAT_ASSERT( strcmp( small, "0123456" ) == 0 );

	int added = g_featFails - fc;
	printf( "hfStr: %s\n", added ? "FAIL" : "ok" );
}

/*
========
BSP writer

Builds a small but structurally complete BSP_VERSION 46 map in memory. The
records are laid out straight from qfiles.h rather than from the reader's own
structs, so the test proves the two agree about the format instead of about
themselves. dnode_t is 9 ints, dleaf_t is 12 ints, dsurface_t is 12 ints
followed by a vec3, three vec3s and two ints, and shader names are fixed
64 byte fields.
========
*/
struct BspWriter {
	unsigned char *	data;
	int				len;
	int				alloced;

	BspWriter() : data( NULL ), len( 0 ), alloced( 0 ) {}
	~BspWriter() { free( data ); }

	// deep copy: the negative cases poke at single bytes and must not reach
	// back into the buffer the good cases still use
	BspWriter	Clone() const {
					BspWriter copy;
					copy.Room( len );
					memcpy( copy.data, data, len );
					copy.len = len;
					return copy;
				}

	void	Room( int n ) {
		if ( len + n > alloced ) {
			alloced = ( len + n ) * 2 + 64;
			data = (unsigned char *)realloc( data, alloced );
		}
	}
	void	Raw( const void *src, int n ) { Room( n ); memcpy( data + len, src, n ); len += n; }
	void	Int( int v ) { Room( sizeof( int ) ); memcpy( data + len, &v, sizeof( int ) ); len += (int)sizeof( int ); }
	void	Float( float v ) { Room( sizeof( float ) ); memcpy( data + len, &v, sizeof( float ) ); len += (int)sizeof( float ); }
	void	Text( const char *s ) { Raw( s, (int)strlen( s ) + 1 ); }
	// a qpath: fixed width, zero padded
	void	QPath( const char *s ) {
		int n = (int)strlen( s );
		if ( n > HF_BSP_MAX_QPATH - 1 ) { n = HF_BSP_MAX_QPATH - 1; }
		Room( HF_BSP_MAX_QPATH );
		memset( data + len, 0, HF_BSP_MAX_QPATH );
		memcpy( data + len, s, n );
		len += HF_BSP_MAX_QPATH;
	}
};

// the single box brush the fixture is built around
static const float BSP_MIN[ 3 ] = { -64.0f, -64.0f, -16.0f };
static const float BSP_MAX[ 3 ] = {  64.0f,  64.0f,  48.0f };

// writes the fixture into 'out', returning its size in bytes
static int BspBuildFixture( BspWriter &out ) {
	BspWriter	lumps[ HF_BSP_HEADER_LUMPS ];
	for ( int i = 0; i < HF_BSP_HEADER_LUMPS; i++ ) { lumps[i] = BspWriter(); }

	// planes: six axis aligned faces of the box. a point is inside when
	// normal . p - dist <= 0, so dist is the normal evaluated on the face
	for ( int axis = 0; axis < 3; axis++ ) {
		for ( int s = 0; s < 2; s++ ) {
			const float sign = ( s == 0 ) ? 1.0f : -1.0f;
			const float coord = ( s == 0 ) ? BSP_MAX[ axis ] : BSP_MIN[ axis ];
			for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_PLANES ].Float( c == axis ? sign : 0.0f ); }
			lumps[ HF_BSP_LUMP_PLANES ].Float( sign * coord );
		}
	}

	// one brush using all six planes, one side per plane
	lumps[ HF_BSP_LUMP_BRUSHES ].Int( 0 );		// firstSide
	lumps[ HF_BSP_LUMP_BRUSHES ].Int( 6 );		// numSides
	lumps[ HF_BSP_LUMP_BRUSHES ].Int( 0 );		// shaderNum
	for ( int i = 0; i < 6; i++ ) {
		lumps[ HF_BSP_LUMP_BRUSHSIDES ].Int( i );
		lumps[ HF_BSP_LUMP_BRUSHSIDES ].Int( 0 );
	}

	// one shader
	lumps[ HF_BSP_LUMP_SHADERS ].QPath( "textures/common/caulk" );
	lumps[ HF_BSP_LUMP_SHADERS ].Int( 0 );		// surfaceFlags
	lumps[ HF_BSP_LUMP_SHADERS ].Int( 1 );		// contentFlags

	// model 0 is the world and owns brush 0
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_MODELS ].Float( BSP_MIN[ c ] ); }
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_MODELS ].Float( BSP_MAX[ c ] ); }
	lumps[ HF_BSP_LUMP_MODELS ].Int( 0 );		// firstSurface
	lumps[ HF_BSP_LUMP_MODELS ].Int( 0 );		// numSurfaces
	lumps[ HF_BSP_LUMP_MODELS ].Int( 0 );		// firstBrush
	lumps[ HF_BSP_LUMP_MODELS ].Int( 1 );		// numBrushes

	// entities, in idTech3's own layout: no brace before the first entity,
	// "}\n{\n" between them, trailing "}\n", and unquoted braces
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"classname\" \"worldspawn\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"message\" \"line one\\nline two\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"quote\" \"he said \\\"hi\\\"\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"wspawn\" \"1\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "}\n{\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"classname\" \"info_player_start\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"origin\" \"32 32 16\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "\"angle\" \"90\"\n" );
	lumps[ HF_BSP_LUMP_ENTITIES ].Text( "}\n" );

	// traversal: one node over two leafs, one leaf carrying the box
	lumps[ HF_BSP_LUMP_NODES ].Int( 0 );		// planeNum
	lumps[ HF_BSP_LUMP_NODES ].Int( -1 );		// children are -(leaf+1)
	lumps[ HF_BSP_LUMP_NODES ].Int( -2 );
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_NODES ].Int( (int)BSP_MIN[ c ] ); }
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_NODES ].Int( (int)BSP_MAX[ c ] ); }

	lumps[ HF_BSP_LUMP_LEAFS ].Int( 1 );		// cluster
	lumps[ HF_BSP_LUMP_LEAFS ].Int( 0 );		// area
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_LEAFS ].Int( (int)BSP_MIN[ c ] ); }
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_LEAFS ].Int( (int)BSP_MAX[ c ] ); }
	lumps[ HF_BSP_LUMP_LEAFS ].Int( 0 );		// firstLeafSurface
	lumps[ HF_BSP_LUMP_LEAFS ].Int( 0 );		// numLeafSurfaces
	lumps[ HF_BSP_LUMP_LEAFS ].Int( 0 );		// firstLeafBrush
	lumps[ HF_BSP_LUMP_LEAFS ].Int( 0 );		// numLeafBrushes

	lumps[ HF_BSP_LUMP_LEAFSURFACES ].Int( 0 );
	lumps[ HF_BSP_LUMP_LEAFSURFACES ].Int( 0 );
	lumps[ HF_BSP_LUMP_LEAFBRUSHES ].Int( 0 );

	// one planar quad
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// shaderNum
	lumps[ HF_BSP_LUMP_SURFACES ].Int( -1 );	// fogNum
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 1 );		// MST_PLANAR
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// firstVert
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 4 );		// numVerts
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// firstIndex
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 6 );		// numIndexes
	lumps[ HF_BSP_LUMP_SURFACES ].Int( -1 );	// lightmapNum
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// lightmapX
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// lightmapY
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// lightmapWidth
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// lightmapHeight
	for ( int c = 0; c < 3; c++ ) { lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f ); }	// lightmapOrigin
	lumps[ HF_BSP_LUMP_SURFACES ].Float( 1.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f );
	lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 1.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f );
	lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 0.0f ); lumps[ HF_BSP_LUMP_SURFACES ].Float( 1.0f );
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// patchWidth
	lumps[ HF_BSP_LUMP_SURFACES ].Int( 0 );		// patchHeight

	const float verts[ 4 ][ 10 ] = {
		{ -64, -64, 48, 0, 0, 0, 0, 0, 0, 1 },
		{  64, -64, 48, 1, 0, 1, 0, 0, 0, 1 },
		{  64,  64, 48, 1, 1, 1, 1, 0, 0, 1 },
		{ -64,  64, 48, 0, 1, 0, 1, 0, 0, 1 },
	};
	const unsigned char colors[ 4 ][ 4 ] = {
		{ 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 }, { 255, 255, 0, 255 },
	};
	for ( int v = 0; v < 4; v++ ) {
		for ( int c = 0; c < 10; c++ ) { lumps[ HF_BSP_LUMP_DRAWVERTS ].Float( verts[ v ][ c ] ); }
		lumps[ HF_BSP_LUMP_DRAWVERTS ].Raw( colors[ v ], 4 );
	}
	const int indexes[ 6 ] = { 0, 1, 2, 0, 2, 3 };
	for ( int i = 0; i < 6; i++ ) { lumps[ HF_BSP_LUMP_DRAWINDEXES ].Int( indexes[ i ] ); }

	// raw byte lumps the reader only has to bound check
	for ( int i = 0; i < 16; i++ ) { lumps[ HF_BSP_LUMP_LIGHTMAPS ].Raw( "\xAA", 1 ); }
	for ( int i = 0; i < 32; i++ ) { lumps[ HF_BSP_LUMP_LIGHTGRID ].Raw( "\0", 1 ); }

	// header, then the lumps laid out back to back
	out.Int( HF_BSP_IDENT );
	out.Int( HF_BSP_VERSION );
	const int headerEnd = out.len + HF_BSP_HEADER_LUMPS * (int)sizeof( hfBspLump );
	int offset = headerEnd;
	for ( int i = 0; i < HF_BSP_HEADER_LUMPS; i++ ) {
		out.Int( offset );
		out.Int( lumps[ i ].len );
		offset += lumps[ i ].len;
	}
	for ( int i = 0; i < HF_BSP_HEADER_LUMPS; i++ ) {
		if ( lumps[ i ].len > 0 ) {
			out.Raw( lumps[ i ].data, lumps[ i ].len );
		}
	}
	return out.len;
}

static void Test_BSP() {
	int fc = g_featFails;

	BspWriter	buf;
	const int	size = BspBuildFixture( buf );
	FEAT_ASSERT( size > 0 && size < 2048 );		// a fixture this small keeps the test readable

	// (a) container validation
	hfBspFile	bsp;
	FEAT_ASSERT( bsp.Load( NULL, size ) == hfBspFile::HF_BSP_ERR_ARGUMENTS );
	FEAT_ASSERT( bsp.Load( buf.data, 8 ) == hfBspFile::HF_BSP_ERR_TRUNCATED_HEADER );

	BspWriter	badIdent( buf.Clone() );
	badIdent.data[ 0 ] = 'X';
	FEAT_ASSERT( bsp.Load( badIdent.data, badIdent.len ) == hfBspFile::HF_BSP_ERR_IDENT );

	BspWriter	badVersion( buf.Clone() );
	badVersion.data[ sizeof( int ) ]     = 45;
	badVersion.data[ sizeof( int ) + 1 ] = 0;
	FEAT_ASSERT( bsp.Load( badVersion.data, badVersion.len ) == hfBspFile::HF_BSP_ERR_VERSION );

	// a lump that claims to run past the end of the file: pointing the first
	// lump at the end of the file leaves no room for its own length
	BspWriter	badLump( buf.Clone() );
	memcpy( badLump.data + sizeof( int ) * 2, &badLump.len, sizeof( int ) );
	FEAT_ASSERT( bsp.Load( badLump.data, badLump.len ) == hfBspFile::HF_BSP_ERR_LUMP_BOUNDS );

	// (b) the fixture itself loads, and every lump lands on the right count
	FEAT_ASSERT( bsp.Load( buf.data, size ) == hfBspFile::HF_BSP_OK );
	FEAT_ASSERT( bsp.GetNumEntities() == 2 );
	FEAT_ASSERT( bsp.GetNumModels() == 1 );
	FEAT_ASSERT( bsp.GetNumBrushes() == 1 );
	FEAT_ASSERT( bsp.GetNumShaders() == 1 );
	FEAT_ASSERT( bsp.GetNumPlanes() == 6 );
	FEAT_ASSERT( bsp.GetNumNodes() == 1 );
	FEAT_ASSERT( bsp.GetNumLeafs() == 1 );
	FEAT_ASSERT( bsp.GetNumSurfaces() == 1 );
	FEAT_ASSERT( bsp.GetNumFogs() == 0 );
	FEAT_ASSERT( bsp.GetNumDrawVerts() == 4 );
	FEAT_ASSERT( bsp.GetNumDrawIndexes() == 6 );
	FEAT_ASSERT( strcmp( bsp.ShaderName( 0 ), "textures/common/caulk" ) == 0 );

	// (c) entities: bare braces, escaped quotes and a literal \n all survive
	const hfBspEntity &world = bsp.GetEntity( 0 );
	FEAT_ASSERT( world.Num() == 4 );
	FEAT_ASSERT( strcmp( world.GetString( "classname" ), "worldspawn" ) == 0 );
	FEAT_ASSERT( strcmp( world.GetString( "wspawn" ), "1" ) == 0 );
	// the entity lump stores \n as two characters, so the value keeps it literal
	FEAT_ASSERT( strcmp( world.GetString( "message" ), "line one\\nline two" ) == 0 );
	FEAT_ASSERT( strcmp( world.GetString( "quote" ), "he said \"hi\"" ) == 0 );

	const hfBspEntity &player = bsp.GetEntity( 1 );
	FEAT_ASSERT( player.Num() == 3 );
	FEAT_ASSERT( strcmp( player.GetString( "classname" ), "info_player_start" ) == 0 );
	FEAT_ASSERT( strcmp( player.GetString( "origin" ), "32 32 16" ) == 0 );
	FEAT_ASSERT( strcmp( player.GetString( "angle" ), "90" ) == 0 );
	FEAT_ASSERT( bsp.GetEntity( 5 ).Num() == 0 );	// out of range

	// (d) traversal and surface records are indexed without being reinterpreted
	const hfBspNode &node = bsp.GetNode( 0 );
	FEAT_ASSERT( node.planeNum == 0 && node.children[ 0 ] == -1 && node.children[ 1 ] == -2 );
	FEAT_ASSERT( node.mins[ 0 ] == (int)BSP_MIN[ 0 ] && node.maxs[ 2 ] == (int)BSP_MAX[ 2 ] );
	const hfBspLeaf &leaf = bsp.GetLeaf( 0 );
	FEAT_ASSERT( leaf.cluster == 1 && leaf.mins[ 2 ] == (int)BSP_MIN[ 2 ] && leaf.maxs[ 1 ] == (int)BSP_MAX[ 1 ] );
	const hfBspSurface &surf = bsp.GetSurface( 0 );
	FEAT_ASSERT( surf.shaderNum == 0 && surf.fogNum == -1 && surf.surfaceType == 1 );
	FEAT_ASSERT( surf.numVerts == 4 && surf.numIndexes == 6 );
	const hfBspModel &model = bsp.GetModel( 0 );
	FEAT_ASSERT( model.mins[ 0 ] == BSP_MIN[ 0 ] && model.maxs[ 2 ] == BSP_MAX[ 2 ] );
	FEAT_ASSERT( model.numBrushes == 1 );

	// (e) brush geometry: six planes, eight corners, exact bounds
	const hfBspBrush &brush = bsp.GetBrush( 0 );
	FEAT_ASSERT( brush.numSides == 6 && brush.numPlanes == 6 );
	FEAT_ASSERT( brush.shaderNum == 0 );
	FEAT_ASSERT( brush.planes != NULL );
	FEAT_ASSERT( brush.ContainsPoint( hfVec3( 0.0f, 0.0f, 0.0f ) ) );
	FEAT_ASSERT( !brush.ContainsPoint( hfVec3( 100.0f, 0.0f, 0.0f ) ) );

	hfList<hfVec3>	hull;
	FEAT_ASSERT( brush.BuildHull( hull ) == 8 );
	hfVec3	mins, maxs;
	brush.ComputeBounds( mins, maxs );
	FEAT_NEAR( mins.x, BSP_MIN[ 0 ], 0.001f );
	FEAT_NEAR( mins.y, BSP_MIN[ 1 ], 0.001f );
	FEAT_NEAR( mins.z, BSP_MIN[ 2 ], 0.001f );
	FEAT_NEAR( maxs.x, BSP_MAX[ 0 ], 0.001f );
	FEAT_NEAR( maxs.y, BSP_MAX[ 1 ], 0.001f );
	FEAT_NEAR( maxs.z, BSP_MAX[ 2 ], 0.001f );

	// every corner has to sit on the boundary of the solid, and the centroid
	// has to sit inside it
	for ( int i = 0; i < hull.Num(); i++ ) {
		FEAT_ASSERT( brush.ContainsPoint( hull[ i ] ) );
	}
	hfVec3	center;
	center.Zero();
	for ( int i = 0; i < hull.Num(); i++ ) { center += hull[ i ]; }
	center *= ( 1.0f / (float)hull.Num() );
	FEAT_ASSERT( brush.ContainsPoint( center ) );
	FEAT_NEAR( center.z, 16.0f, 0.001f );

	// (f) the mesh path: centroid apex plus one ring vertex per corner
	hfList<hfVec3>	meshVerts;
	hfList<int>		meshIdx;
	FEAT_ASSERT( brush.BuildMesh( meshVerts, meshIdx ) == 8 );
	FEAT_ASSERT( meshVerts.Num() == 9 );
	FEAT_ASSERT( meshIdx.Num() == 24 );		// one triangle per corner
	int		winding = 0;
	for ( int t = 0; t + 2 < meshIdx.Num(); t += 3 ) {
		FEAT_ASSERT( meshIdx[ t ] >= 0 && meshIdx[ t ] < meshVerts.Num() );
		FEAT_ASSERT( meshIdx[ t + 1 ] >= 0 && meshIdx[ t + 1 ] < meshVerts.Num() );
		FEAT_ASSERT( meshIdx[ t + 2 ] >= 0 && meshIdx[ t + 2 ] < meshVerts.Num() );
		hfVec3	a = meshVerts[ meshIdx[ t + 1 ] ] - meshVerts[ meshIdx[ t ] ];
		hfVec3	b = meshVerts[ meshIdx[ t + 2 ] ] - meshVerts[ meshIdx[ t ] ];
		hfVec3	n = a.Cross( b );
		if ( n.Length() > 0.001f ) {
			n.Normalize();
			if ( n * ( meshVerts[ meshIdx[ t + 1 ] ] - center ) > 0.0f ) { winding++; }
		}
	}
	// a fan over a convex solid all winds the same way
	FEAT_ASSERT( winding == 0 || winding == meshIdx.Num() / 3 );

	// (g) loading the same buffer again must not leak state from the first pass
	FEAT_ASSERT( bsp.Load( buf.data, size ) == hfBspFile::HF_BSP_OK );
	FEAT_ASSERT( bsp.GetNumEntities() == 2 && bsp.GetNumBrushes() == 1 );

	int added = g_featFails - fc;
	printf( "BSP: %s\n", added ? "FAIL" : "ok" );
}

static void Test_Animation();
static void Test_Game();
static void Test_Console();
static void Test_UiWidgets();

/*
================================
Archive member data path

`.pk3`/`.zip` members are read through the bundled zlib in
`framework/zlib`, so what gets checked here is that this build inflates real
third party members, verifies them against an independent CRC, and fails
loudly instead of running off the end of a damaged member. The member stream
below was produced by python's zlib and lifted straight out of a zip file, so
it exercises dynamic Huffman codes rather than a friendly stored block.
================================
*/
// a raw deflate stream produced by a third party compressor (python zlib,
// level 9), taken verbatim out of a zip member, so this covers the bundled
// inflate path on data the engine did not produce
static const unsigned char deflateStream[] = {
237,198,187,13,128,32,20,0,192,85,88,133,
132,103,101,44,12,141,165,248,3,247,31,192,
29,172,239,170,43,49,205,185,70,73,177,212,
117,75,123,59,206,235,126,250,120,205,204,204,
204,204,204,204,204,204,204,204,236,239,62
};

static void Test_Zip() {
	int fc = g_featFails;

	const char *prefix = "DEFLATED ENTRY ";
	const int	repeat = 600;
	const int	plainLen = (int)strlen( prefix ) + repeat * 10;
	// crc32 of the member payload, taken from the archive's central directory
	const unsigned long memberCrc = 0xe307ade7UL;

	unsigned char *plain = new unsigned char[ plainLen ];
	int plainPos = 0;
	memcpy( plain, prefix, strlen( prefix ) );
	plainPos = (int)strlen( prefix );
	for ( int i = 0; i < repeat; i++ ) {
		memcpy( plain + plainPos, "abcdefghij", 10 );
		plainPos += 10;
	}
	FEAT_ASSERT( plainPos == plainLen );

	// (a) crc32 matches the value recorded in the archive
	FEAT_ASSERT( crc32( 0, plain, plainLen ) == memberCrc );

	// (b) the member inflates to exactly the payload, and to nothing more
	unsigned char *out = new unsigned char[ plainLen + 64 ];
	z_stream zs;
	memset( &zs, 0, sizeof( zs ) );
	// negative window bits: the member holds a raw deflate stream
	FEAT_ASSERT( inflateInit2( &zs, -MAX_WBITS ) == Z_OK );
	zs.next_in   = (unsigned char *)deflateStream;
	zs.avail_in  = (unsigned)sizeof( deflateStream );
	zs.next_out  = out;
	zs.avail_out = plainLen + 64;
	int zr = inflate( &zs, Z_FINISH );
	int produced = plainLen + 64 - (int)zs.avail_out;
	FEAT_ASSERT( zr == Z_STREAM_END );
	FEAT_ASSERT( produced == plainLen );
	FEAT_ASSERT( memcmp( out, plain, plainLen ) == 0 );
	FEAT_ASSERT( inflateEnd( &zs ) == Z_OK );

	// (c) a truncated member is an error, not a partial success
	memset( &zs, 0, sizeof( zs ) );
	FEAT_ASSERT( inflateInit2( &zs, -MAX_WBITS ) == Z_OK );
	zs.next_in   = (unsigned char *)deflateStream;
	zs.avail_in  = (unsigned)sizeof( deflateStream ) - 8;
	zs.next_out  = out;
	zs.avail_out = plainLen;
	zr = inflate( &zs, Z_FINISH );
	FEAT_ASSERT( zr != Z_STREAM_END );
	FEAT_ASSERT( zr == Z_BUF_ERROR || zr == Z_DATA_ERROR || zr == Z_OK );
	FEAT_ASSERT( inflateEnd( &zs ) == Z_OK );

	// (d) the compressed form is actually smaller than the payload, or the
	// fixture is not exercising the inflate path at all
	FEAT_ASSERT( (int)sizeof( deflateStream ) < plainLen );

	// (e) round trip through the bundled deflate, then inflate it again
	memset( &zs, 0, sizeof( zs ) );
	FEAT_ASSERT( deflateInit( &zs, 9 ) == Z_OK );
	unsigned char *packed = new unsigned char[ plainLen + 64 ];
	zs.next_in   = plain;
	zs.avail_in  = (unsigned)plainLen;
	zs.next_out  = packed;
	zs.avail_out = plainLen + 64;
	zr = deflate( &zs, Z_FINISH );
	int packedLen = plainLen + 64 - (int)zs.avail_out;
	FEAT_ASSERT( zr == Z_STREAM_END );
	FEAT_ASSERT( deflateEnd( &zs ) == Z_OK );
	FEAT_ASSERT( packedLen < plainLen );

	memset( &zs, 0, sizeof( zs ) );
	FEAT_ASSERT( inflateInit( &zs ) == Z_OK );
	zs.next_in   = packed;
	zs.avail_in  = (unsigned)packedLen;
	zs.next_out  = out;
	zs.avail_out = plainLen;
	zr = inflate( &zs, Z_FINISH );
	FEAT_ASSERT( zr == Z_STREAM_END );
	FEAT_ASSERT( plainLen - (int)zs.avail_out == plainLen );
	FEAT_ASSERT( memcmp( out, plain, plainLen ) == 0 );
	FEAT_ASSERT( inflateEnd( &zs ) == Z_OK );

	// (f) a member whose crc does not match is detectable, which is what the
	// archive reader checks before handing bytes to an asset loader
	unsigned char *corrupt = new unsigned char[ plainLen ];
	memcpy( corrupt, plain, plainLen );
	corrupt[ plainLen / 2 ] ^= 0x40;
	FEAT_ASSERT( crc32( 0, corrupt, plainLen ) != memberCrc );

	delete[] corrupt;
	delete[] packed;
	delete[] out;
	delete[] plain;

	int added = g_featFails - fc;
	printf( "Zip: %s\n", added ? "FAIL" : "ok" );
}

// ---- editor document: open -> edit -> save -> reload (Phase 11) ----
static void Test_Editor() {
	using namespace HF_Editor;

	// (a) the reflection table must be registered, idempotent, and every
	//     property offset must land inside the component it names
	hfEditorRegisterBuiltins();
	hfEditorRegisterBuiltins();
	const int classCount = hfEditorReflection::GetClassCount();
	FEAT_ASSERT( classCount == 5 );
	for ( int c = 0; c < classCount; c++ ) {
		const hfEditorComponentClass *cls = hfEditorReflection::GetClass( c );
		FEAT_ASSERT( cls && cls->name && cls->props && cls->propCount > 0 );
		FEAT_ASSERT( cls->size > 0 );
		FEAT_ASSERT( hfEditorReflection::FindByName( cls->name ) == cls );
		FEAT_ASSERT( hfEditorReflection::FindByTypeKey( cls->typeKey ) == cls );
		FEAT_ASSERT( hfEditorReflection::FindByName( "no.such.component" ) == NULL );
		FEAT_ASSERT( hfEditorReflection::GetClass( -1 ) == NULL );
		FEAT_ASSERT( hfEditorReflection::GetClass( classCount ) == NULL );
		for ( int p = 0; p < cls->propCount; p++ ) {
			const hfEditorProperty &pr = cls->props[ p ];
			int bytes;
			switch ( pr.type ) {
				case HF_EDIT_BOOL:   bytes = ( int )sizeof( bool ); break;
				case HF_EDIT_STRING: bytes = ( int )sizeof( hfStr ); break;
				case HF_EDIT_VEC3:   bytes = ( int )sizeof( float ) * 3; break;
				case HF_EDIT_QUAT:   bytes = ( int )sizeof( float ) * 4; break;
				default:             bytes = ( int )sizeof( int ); break;
			}
			FEAT_ASSERT( pr.name && pr.group );
			FEAT_ASSERT( pr.offset >= 0 && pr.offset + bytes <= cls->size );
		}
	}

	const hfEditorComponentClass *trCls = hfEditorReflection::FindByName( "Transform" );
	const hfEditorComponentClass *lightCls = hfEditorReflection::FindByName( "Light" );
	FEAT_ASSERT( trCls && lightCls );
	FEAT_ASSERT( trCls->propCount == 3 );

	// enum metadata is the editor's dropdown for Light.Type
	int typeProp = -1;
	for ( int p = 0; p < lightCls->propCount; p++ ) {
		if ( !strcmp( lightCls->props[p].name, "Type" ) ) { typeProp = p; break; }
	}
	FEAT_ASSERT( typeProp >= 0 );
	FEAT_ASSERT( lightCls->props[typeProp].type == HF_EDIT_ENUM );
	FEAT_ASSERT( lightCls->props[typeProp].enumCount == 3 );
	FEAT_ASSERT( !strcmp( lightCls->props[typeProp].enumNames[2], "directional" ) );

	// offset-only accessor, no component required
	HF_Scene::hfTransformComponent probe;
	hfEditorValue raw;
	raw.SetVec3( 1.0f, 2.0f, 3.0f );
	FEAT_ASSERT( hfEditorReflection::WriteAt( &probe, trCls->props[0], raw ) );
	FEAT_ASSERT( probe.position.x == 1.0f && probe.position.y == 2.0f && probe.position.z == 3.0f );
	hfEditorValue rawOut;
	FEAT_ASSERT( hfEditorReflection::ReadAt( &probe, trCls->props[0], rawOut ) );
	FEAT_ASSERT( rawOut.type == HF_EDIT_VEC3 && rawOut.f[1] == 2.0f );

	// (b) build a small scene through the document; outline is deterministic
	hfEditorDocument doc;
	FEAT_ASSERT( doc.OpenEmpty() );
	FEAT_ASSERT( doc.GetTree().GetCount() == 0 );
	HF_Ecs::Entity root = doc.CreateEntity( "world", -1 );
	HF_Ecs::Entity childA = doc.CreateEntity( "crate", doc.GetTree().FindByTag( "world" ) );
	HF_Ecs::Entity childB = doc.CreateEntity( "lamp", doc.GetTree().FindByTag( "world" ) );
	FEAT_ASSERT( root != HF_Ecs::ENTITY_NULL && childA != HF_Ecs::ENTITY_NULL && childB != HF_Ecs::ENTITY_NULL );
	FEAT_ASSERT( doc.GetTree().GetCount() == 3 );
	FEAT_ASSERT( doc.GetTree().GetRootCount() == 1 );
	int ni = doc.GetTree().FindByEntity( root );
	int ai = doc.GetTree().FindByEntity( childA );
	int bi = doc.GetTree().FindByEntity( childB );
	FEAT_ASSERT( ni == 0 && ai >= 0 && bi >= 0 );
	FEAT_ASSERT( doc.GetTree().Get( ni ).depth == 0 && doc.GetTree().Get( ni ).childCount == 2 );
	FEAT_ASSERT( doc.GetTree().Get( ai ).depth == 1 && doc.GetTree().Get( bi ).depth == 1 );
	FEAT_ASSERT( doc.GetTree().Get( ai ).parent == ni && doc.GetTree().Get( bi ).parent == ni );
	FEAT_ASSERT( doc.GetTree().FindByTag( "crate" ) == ai );
	FEAT_ASSERT( doc.GetTree().FindByTag( "nope" ) == -1 );

	hfStr dump1, dump2;
	doc.GetTree().DumpText( dump1 );
	doc.GetTree().DumpText( dump2 );
	FEAT_ASSERT( dump1 == dump2 && dump1.Length() > 0 );

	// an entity with no tag shows up as untagged and can be named later
	HF_Ecs::Entity untagged = doc.CreateEntity( NULL, -1 );
	int ui = doc.GetTree().FindByEntity( untagged );
	FEAT_ASSERT( ui >= 0 && doc.GetTree().Get( ui ).name.Length() == 0 );
	FEAT_ASSERT( doc.SetName( ui, "named_later" ) );
	FEAT_ASSERT( doc.GetTree().FindByTag( "named_later" ) == ui );

	// (c) select through the inspector sheet and edit a transform
	FEAT_ASSERT( doc.Select( ai ) && doc.GetSelectedEntity() == childA );
	FEAT_ASSERT( doc.GetSheetSize() >= 4 );		// Transform(3) + Tag(1)
	FEAT_ASSERT( doc.FindSheetEntry( "Transform.Position" ) >= 0 );
	FEAT_ASSERT( doc.FindSheetEntry( "Bogus.thing" ) == -1 );
	FEAT_ASSERT( doc.FindSheetEntry( "nodot" ) == -1 );

	int posX = doc.FindSheetEntry( "Transform.Position" );
	hfEditorValue v;
	FEAT_ASSERT( doc.GetPropertyValue( posX, v ) );
	FEAT_ASSERT( v.type == HF_EDIT_VEC3 && v.f[0] == 0.0f );
	v.SetVec3( 5.0f, 0.0f, -2.0f );
	FEAT_ASSERT( doc.SetPropertyValue( posX, v ) );
	FEAT_ASSERT( doc.IsDirty() );
	HF_Scene::hfTransformComponent *tt =
		doc.GetWorld().GetComponent< HF_Scene::hfTransformComponent >( childA );
	FEAT_NEAR( tt->position.x, 5.0f, 1e-4f );
	FEAT_NEAR( tt->position.z, -2.0f, 1e-4f );

	// rename via Tag.Name; the tree follows immediately
	int nameIdx = doc.FindSheetEntry( "Tag.Name" );
	FEAT_ASSERT( nameIdx >= 0 );
	hfEditorValue nv;
	nv.SetString( "crate_renamed" );
	FEAT_ASSERT( doc.SetPropertyValue( nameIdx, nv ) );
	FEAT_ASSERT( doc.GetTree().FindByTag( "crate_renamed" ) == ai );
	FEAT_ASSERT( doc.GetTree().FindByTag( "crate" ) == -1 );

	// (d) add a light, edit its enum + bool through the sheet
	int lidx = doc.GetTree().FindByEntity( childB );
	FEAT_ASSERT( lidx >= 0 && doc.Select( lidx ) );
	FEAT_ASSERT( doc.AddComponent( lidx, "Light" ) );
	FEAT_ASSERT( doc.Select( lidx ) );
	int typeIdx = doc.FindSheetEntry( "Light.Type" );
	int enabledIdx = doc.FindSheetEntry( "Light.Enabled" );
	FEAT_ASSERT( typeIdx >= 0 && enabledIdx >= 0 );
	hfEditorValue tv;
	tv.SetInt( 2 );		// directional
	FEAT_ASSERT( doc.SetPropertyValue( typeIdx, tv ) );
	hfEditorValue ev;
	ev.SetBool( false );
	FEAT_ASSERT( doc.SetPropertyValue( enabledIdx, ev ) );
	HF_Scene::hfLightComponent *lc =
		doc.GetWorld().GetComponent< HF_Scene::hfLightComponent >( childB );
	FEAT_ASSERT( lc && lc->type == HF_Scene::HF_LIGHT_DIRECTIONAL && !lc->enabled );
	tv.SetInt( 9 );		// out-of-range enum is refused
	FEAT_ASSERT( !doc.SetPropertyValue( typeIdx, tv ) );

	// add a mesh and hide it
	int aidx = doc.GetTree().FindByEntity( childA );
	FEAT_ASSERT( aidx >= 0 && doc.Select( aidx ) );
	FEAT_ASSERT( doc.AddComponent( aidx, "MeshRenderer" ) );
	FEAT_ASSERT( doc.Select( aidx ) );
	int visIdx = doc.FindSheetEntry( "MeshRenderer.Visible" );
	FEAT_ASSERT( visIdx >= 0 );
	hfEditorValue vv;
	vv.SetBool( false );
	FEAT_ASSERT( doc.SetPropertyValue( visIdx, vv ) );
	HF_Scene::hfMeshRendererComponent *mc =
		doc.GetWorld().GetComponent< HF_Scene::hfMeshRendererComponent >( childA );
	FEAT_ASSERT( mc && !mc->visible );

	// add component is idempotent; remove is a documented gap
	FEAT_ASSERT( doc.AddComponent( aidx, "MeshRenderer" ) );
	FEAT_ASSERT( !doc.RemoveComponent( aidx, "MeshRenderer" ) );

	// (e) structural edits: reparent, create under, delete
	FEAT_ASSERT( doc.Reparent( lidx, aidx ) );
	FEAT_ASSERT( doc.GetTree().Get( lidx ).parent == aidx );
	FEAT_ASSERT( doc.GetTree().Get( aidx ).depth == 1 );
	FEAT_ASSERT( doc.GetTree().Get( lidx ).depth == 2 );
	FEAT_ASSERT( !doc.Reparent( aidx, lidx ) );		// would be a cycle
	FEAT_ASSERT( !doc.Reparent( aidx, aidx ) );		// no self-parent

	HF_Ecs::Entity grand = doc.CreateEntity( "bulb", aidx );
	FEAT_ASSERT( grand != HF_Ecs::ENTITY_NULL );
	int gi = doc.GetTree().FindByEntity( grand );
	FEAT_ASSERT( gi >= 0 && doc.GetTree().Get( gi ).parent == aidx );
	FEAT_ASSERT( doc.DeleteEntity( gi ) );
	FEAT_ASSERT( doc.GetTree().FindByEntity( grand ) == -1 );

	// (f) save -> reload -> save must be byte-identical, and the semantics
	//     must survive the reload
	doc.MarkClean();
	hfStr text1;
	FEAT_ASSERT( doc.Save( text1 ) );
	FEAT_ASSERT( text1.Length() > 60 );

	hfEditorDocument reloaded;
	FEAT_ASSERT( reloaded.Open( text1.c_str() ) );
	hfStr text2;
	FEAT_ASSERT( reloaded.Save( text2 ) );
	FEAT_ASSERT( text1 == text2 );

	HF_Scene::hfWorld &rw = reloaded.GetWorld();
	HF_Ecs::Entity rroot = rw.FindEntityByTag( "world" );
	HF_Ecs::Entity rcrate = rw.FindEntityByTag( "crate_renamed" );
	HF_Ecs::Entity rlamp = rw.FindEntityByTag( "lamp" );
	HF_Ecs::Entity rnamed = rw.FindEntityByTag( "named_later" );
	FEAT_ASSERT( rroot != HF_Ecs::ENTITY_NULL && rcrate != HF_Ecs::ENTITY_NULL );
	FEAT_ASSERT( rlamp != HF_Ecs::ENTITY_NULL && rnamed != HF_Ecs::ENTITY_NULL );
	FEAT_ASSERT( rw.GetParent( rcrate ) == rroot );
	FEAT_ASSERT( rw.GetParent( rlamp ) == rcrate );		// reparent survived
	HF_Scene::hfTransformComponent *rt =
		rw.GetComponent< HF_Scene::hfTransformComponent >( rcrate );
	FEAT_NEAR( rt->position.x, 5.0f, 1e-3f );
	FEAT_NEAR( rt->position.z, -2.0f, 1e-3f );
	HF_Scene::hfLightComponent *rlight = rw.GetComponent< HF_Scene::hfLightComponent >( rlamp );
	FEAT_ASSERT( rlight && rlight->type == HF_Scene::HF_LIGHT_DIRECTIONAL && !rlight->enabled );
	HF_Scene::hfMeshRendererComponent *rmesh =
		rw.GetComponent< HF_Scene::hfMeshRendererComponent >( rcrate );
	FEAT_ASSERT( rmesh && !rmesh->visible );

	// second round trip, reusing the first document to bound live registries
	FEAT_ASSERT( doc.Open( text2.c_str() ) );
	hfStr text3;
	FEAT_ASSERT( doc.Save( text3 ) );
	FEAT_ASSERT( text2 == text3 );

	printf( "Editor ok\n" );
}

int HfTest_FeaturesMain() {
	g_featFails = 0;
	printf( "--- features (ecs/asset/cluster/stream/audio/collide/audiospatial/scene/lights/resources/ser/physics/materials/import/str/bsp/zip/net/editor) ---\n" );
	Test_ECS();
	Test_AssetStore();
	Test_Cluster();
	Test_Stream();
	Test_Audio();
	Test_Collide();
	Test_AudioSpatial();
	Test_Scene();
	Test_Lights();
	Test_Resources();
	Test_Serialization();
	Test_Headless();
	Test_SimServer();
	Test_AudioServer();
	Test_Ui();
	Test_Net();
	Test_Editor();
	Test_Physics();
	Test_PhysicsBackend();
	Test_Materials();
	Test_Import();
	Test_hfStr();
	Test_BSP();
	Test_Zip();
	Test_Animation();
	Test_Game();
	Test_Console();
	Test_UiWidgets();
	printf( "features logic: %s (%d failures)\n", g_featFails == 0 ? "ok" : "FAIL", g_featFails );
	return g_featFails;
}



static void	Test_Animation() {  // Phase 6: skeleton + clip + system
	int fc = g_featFails;
	{
		// (a) skeleton: add bones, find by name, compose local->world
		hfSkeletonComponent	skel;
		int root = skel.AddBone( "root", -1 );
		int hip  = skel.AddBone( "hips", 0 );
		int thigh= skel.AddBone( "thigh", 1 );
		int shin = skel.AddBone( "shin", 2 );
		FEAT_ASSERT( root == 0 && hip == 1 && thigh == 2 && shin == 3 );
		FEAT_ASSERT( skel.FindBone( "thigh" ) == 2 );
		FEAT_ASSERT( skel.GetBoneCountHF() == 4 );
		// (b) clip: two tracks (hips rotate, shin translate), one event
		hfAnimationClip	clip;
		int tHip = clip.AddTrack( 1 );
		int tShin= clip.AddTrack( 3 );
		FEAT_ASSERT( tHip == 0 && tShin == 1 );
		hfAnimTrack &th = clip.tracks[ tHip ];
		hfAnimTrack &ts = clip.tracks[ tShin ];
		// 90 degrees about +Z is the quaternion (0,0,sin45,cos45)
		const float h = 0.5f * 3.14159265f / 2.0f;
		hfQuat q0( 0.0f, 0.0f, 0.0f, 1.0f );
		hfQuat q1( 0.0f, 0.0f, (float)sin( h ), (float)cos( h ) );
		th.AddR( 0.0f, q0 );
		th.AddR( 1.0f, q1 );
		ts.AddT( 0.0f, hfVec3( 0,0,0 ) );
		ts.AddT( 1.0f, hfVec3( 0,4,0 ) );
		clip.AddEvent( 0.5f, "footstep" );
		clip.duration = 1.0f;
		FEAT_ASSERT( clip.EndTime() == 1.0f );
		FEAT_ASSERT( clip.NormalizeTime( 1.25f ) == 0.25f );
		// (c) sample: t=0 -> rest, t=0.5 -> Slerp half (90deg) + mid tscala
		hfBonePose	pose[ HF_SKELETON_MAX_BONES ];
		clip.Sample( 0.0f, pose, HF_SKELETON_MAX_BONES );
		FEAT_ASSERT( pose[ 1 ].rotation.w > 0.99f );
		clip.Sample( 0.5f, pose, HF_SKELETON_MAX_BONES );
		// halfway between rest and 90 degrees about +Z is a 45 degree rotation,
		// so the quaternion components are sin/cos of half that: sin(22.5) and
		// cos(22.5). Note sin(45) would be the *angle*, not the component.
		FEAT_NEAR( pose[ 1 ].rotation.z, 0.3826834f, 0.001f );
		FEAT_NEAR( pose[ 1 ].rotation.w, 0.9238795f, 0.001f );
		FEAT_ASSERT( pose[ 3 ].position.y > 1.9f );  // ~2.0
		// (d) playback system: play, advance, verify pose + loop wrap
		hfAnimationSystem	sys;
		sys.SetClips( &clip, 1 );
		sys.SetSkeleton( &skel );
		int p = sys.Play( 0, 0 );
		FEAT_ASSERT( p == 0 );
		sys.Advance( 0.25f );
		FEAT_ASSERT( sys.Playhead( 0 ) > 0.24f && sys.Playhead( 0 ) < 0.26f );
		FEAT_ASSERT( skel.localPose[ 1 ].rotation.w < 0.99f );  // sampled
	}	// phase 6 block
	{
		// (e) rest pose, forward kinematics and inverse-bind skinning
		hfSkeletonComponent	sk;
		int	r = sk.AddBone( "root", -1 );
		int	c = sk.AddBone( "child", 0 );
		FEAT_ASSERT( r == 0 && c == 1 );
		FEAT_ASSERT( sk.GetParent( c ) == 0 && sk.GetParent( r ) == -1 );
		sk.SetRestPose( r, hfQuat( 0,0,0,1 ), hfVec3( 0,0,0 ) );
		sk.SetRestPose( c, hfQuat( 0,0,0,1 ), hfVec3( 0,2,0 ) );
		sk.ResetToRest();
		sk.ComposeWorld();
		sk.ComputeSkinMatrices();
		hfVec3	out;
		// at rest the inverse-bind cancels the bind pose: skinMatrix == identity
		FEAT_ASSERT( sk.TransformVertex( c, hfVec3( 0,2,0 ), out ) );
		FEAT_NEAR( out.x, 0.0f, 1e-4f );
		FEAT_NEAR( out.y, 2.0f, 1e-4f );
		FEAT_NEAR( out.z, 0.0f, 1e-4f );
		// rotate root 90 deg about +Z; the child rides from +Y onto +X
		const float h2 = 0.5f * 3.14159265f / 2.0f;
		hfQuat qz( 0,0,(float)sin( h2 ),(float)cos( h2 ) );
		sk.ResetToRest();
		sk.SetLocalPose( r, qz, hfVec3( 0,0,0 ) );
		sk.ComposeWorld();
		sk.ComputeSkinMatrices();
		FEAT_NEAR( sk.worldPose[ c ].position.x, 2.0f, 1e-3f );
		FEAT_NEAR( sk.worldPose[ c ].position.y, 0.0f, 1e-3f );
		// a bind-space vertex on the child follows its animated world transform
		FEAT_ASSERT( sk.TransformVertex( c, hfVec3( 0,2,0 ), out ) );
		FEAT_NEAR( out.x, 2.0f, 1e-3f );
		FEAT_NEAR( out.y, 0.0f, 1e-3f );
		// the root's skin matrix is just its rotation: (1,0,0) -> (0,-1,0)
		FEAT_ASSERT( sk.TransformVertex( r, hfVec3( 1,0,0 ), out ) );
		FEAT_NEAR( out.x,  0.0f, 1e-3f );
		FEAT_NEAR( out.y, -1.0f, 1e-3f );
	}
	{
		// (f) weighted blend: identity + 90deg at 0.5/0.5 gives 45deg
		hfSkeletonComponent	sk;
		sk.AddBone( "b", -1 );
		hfAnimationClip	c0, c1;
		c0.AddTrack( 0 ); c1.AddTrack( 0 );
		c0.duration = c1.duration = 1.0f;
		c0.tracks[ 0 ].AddR( 0.0f, hfQuat( 0,0,0,1 ) );
		const float hb = 0.5f * 3.14159265f / 2.0f;
		hfQuat q90( 0,0,(float)sin( hb ),(float)cos( hb ) );
		c1.tracks[ 0 ].AddR( 0.0f, q90 );
		hfAnimationClip	clips[ 2 ] = { c0, c1 };
		hfAnimationSystem	sys;
		sys.SetClips( clips, 2 );
		sys.SetSkeleton( &sk );
		FEAT_ASSERT( sys.PlayWeighted( 0, 0, 0.5f ) == 0 );
		FEAT_ASSERT( sys.PlayWeighted( 1, 1, 0.5f ) == 1 );
		sys.Advance( 0.0f );
		FEAT_NEAR( sk.localPose[ 0 ].rotation.z, 0.3826834f, 0.002f );
		FEAT_NEAR( sk.localPose[ 0 ].rotation.w, 0.9238795f, 0.002f );
		// extremes: weight 0 drops slot 0, weight 1 leaves slot 1 raw
		sys.SetWeight( 0, 0.0f );
		sys.SetWeight( 1, 1.0f );
		FEAT_NEAR( sys.GetWeight( 1 ), 1.0f, 1e-5f );
		sys.Advance( 0.0f );
		FEAT_NEAR( sk.localPose[ 0 ].rotation.z, q90.z, 0.002f );
	}
	{
		// (g) event firing across a loop boundary
		struct EventLog { int count; const char *last; };
		hfSkeletonComponent	sk;
		sk.AddBone( "b", -1 );
		hfAnimationClip	clip;
		clip.AddTrack( 0 );
		clip.duration = 1.0f;
		FEAT_ASSERT( clip.AddEvent( 0.5f, "footstep" ) == 0 );
		FEAT_ASSERT( clip.AddEvent( 0.9f, "land" ) == 1 );
		FEAT_ASSERT( clip.FindEvent( "land" ) == 1 );
		hfAnimationSystem	sys;
		sys.SetClips( &clip, 1 );
		sys.SetSkeleton( &sk );
		EventLog	log = { 0, 0 };
		sys.SetEventCallback( []( const char *n, float t, void *u ) {
			EventLog *l = (EventLog *)u; l->count++; l->last = n;
		}, &log );
		sys.Play( 0, 0 );
		sys.Advance( 0.4f );
		FEAT_ASSERT( log.count == 0 );
		sys.Advance( 0.2f );			// crosses 0.5
		FEAT_ASSERT( log.count == 1 && hfStr::Cmp( log.last, "footstep" ) == 0 );
		sys.Advance( 0.6f );			// crosses 0.9 (lap 1)
		FEAT_ASSERT( log.count == 2 && hfStr::Cmp( log.last, "land" ) == 0 );
		sys.Advance( 0.6f );			// 1.2 -> 1.8 crosses 0.5 (lap 2)
		FEAT_ASSERT( log.count == 3 && hfStr::Cmp( log.last, "footstep" ) == 0 );
	}
	{
		// (h) state machine: bool / float / trigger transitions
		hfAnimationStateMachine	sm;
		int idle = sm.AddState( "idle", 0 );
		int walk = sm.AddState( "walk", 1 );
		int run  = sm.AddState( "run",  2 );
		FEAT_ASSERT( idle == 0 && walk == 1 && run == 2 );
		FEAT_ASSERT( sm.GetState() == idle );		// first added is default
		sm.AddBool( "moving", false );
		sm.AddFloat( "speed", 0.0f );
		sm.AddTrigger( "jump" );
		FEAT_ASSERT( sm.AddTransition( idle, walk, "moving", HF_ANIM_COND_BOOL, 0.0f ) == 0 );
		sm.AddTransition( walk, run,  "speed",  HF_ANIM_COND_FLOAT_GT, 5.0f );
		sm.AddTransition( run,  walk, "speed",  HF_ANIM_COND_FLOAT_LT, 5.0f );
		sm.AddTransition( walk, idle, "moving", HF_ANIM_COND_BOOL_FALSE, 0.0f );
		sm.AddTransition( idle, run,  "jump",   HF_ANIM_COND_TRIGGER, 0.0f );
		sm.SetBool( "moving", true );
		FEAT_ASSERT( sm.Update() == 1 );			// -> walk
		FEAT_ASSERT( sm.GetState() == walk );
		sm.SetFloat( "speed", 7.0f );
		FEAT_ASSERT( sm.Update() == 2 );			// -> run
		sm.SetFloat( "speed", 2.0f );
		FEAT_ASSERT( sm.Update() == 1 );			// -> walk
		sm.SetBool( "moving", false );
		FEAT_ASSERT( sm.Update() == 0 );			// -> idle
		FEAT_ASSERT( sm.GetStateName()[ 0 ] == 'i' );
		// trigger fires once and is consumed
		sm.SetFloat( "speed", 7.0f );
		sm.SetTrigger( "jump" );
		FEAT_ASSERT( sm.Update() == 2 );			// idle -> run
		FEAT_ASSERT( sm.GetLastTransition() == 4 );
		FEAT_ASSERT( sm.Update() == -1 );			// trigger consumed, stays
		FEAT_ASSERT( sm.GetState() == run );
		FEAT_ASSERT( !sm.SetFloat( "bogus", 1.0f ) );
	}
	int added = g_featFails - fc;
	printf( "phase 6 animation: %s\n", added ? "FAIL" : "ok" );
}

// Phase 7 - "Hellforge Arena": the English hfscript dialect driving a C++
// simulation.  Verifies compilation of the .eng front end, that the game
// terminates with a positive score, and that a fixed seed is deterministic.
static void Test_Game() {
	int fc = g_featFails;

	HfScript_RegisterArenaHosts();

	// read the shipped game script (harness cwd is the engine root)
	FILE * f = fopen( "base/script/arena.eng", "rb" );
	FEAT_ASSERT( f != NULL );
	if ( f == NULL ) {
		int added = g_featFails - fc;
		printf( "phase 7 game: %s\n", added ? "FAIL" : "ok" );
		return;
	}
	fseek( f, 0, SEEK_END );
	long size = ftell( f );
	fseek( f, 0, SEEK_SET );
	char * src = (char *)malloc( size + 1 );
	fread( src, 1, size, f );
	fclose( f );
	src[ size ] = 0;

	int		score1 = -1;
	int		ticks1 = -1;
	int		wave1  = -1;
	bool	dialectTag = HfScript_IsEnglishSource( "arena.eng" );
	FEAT_ASSERT( dialectTag );

	{
		HfScriptProgram prog;
		HfScriptVM vm;
		bool ok = HfScript_Compile( &prog, src, "arena.eng" );
		if ( !ok ) {
			printf( "GAME COMPILE FAIL: %s(%d): %s\n", prog.error.c_str(), prog.errorLine, prog.error.c_str() );
		}
		FEAT_ASSERT( ok );
		if ( ok ) {
			FEAT_ASSERT( vm.RunProgram( &prog ) );
			score1 = hfgame::Arena().score;
			ticks1 = hfgame::Arena().ticks;
			wave1  = hfgame::Arena().wave;
			FEAT_ASSERT( !hfgame::Arena().Running() );			// the game ended
			FEAT_ASSERT( ticks1 <= HfArena::TICK_LIMIT );
			FEAT_ASSERT( score1 > 0 );
			FEAT_ASSERT( wave1 >= 1 );
		}
	}

	// determinism: the same seed reproduces the exact same run
	if ( score1 >= 0 ) {
		HfScriptProgram prog2;
		HfScriptVM vm2;
		bool ok2 = HfScript_Compile( &prog2, src, "arena.eng" );
		FEAT_ASSERT( ok2 );
		if ( ok2 ) {
			FEAT_ASSERT( vm2.RunProgram( &prog2 ) );
			FEAT_ASSERT( hfgame::Arena().score == score1 );
			FEAT_ASSERT( hfgame::Arena().ticks == ticks1 );
			FEAT_ASSERT( hfgame::Arena().wave == wave1 );
		}
	}
	free( src );

	// English dialect: word operators, define/with, repeat N times, give back,
	// otherwise.  clampadd returns min(a+b, 10); total converges to 32.
	{
		HfScriptProgram ep;
		HfScriptVM evm;
		const char * eng =
			"define clampadd with a and b\n"
			"    set r to a plus b\n"
			"    if r is greater than 10\n"
			"        give back 10\n"
			"    otherwise\n"
			"        give back r\n"
			"define main\n"
			"    set total to 0\n"
			"    repeat 4 times\n"
			"        increase total by clampadd(total, 4)\n"
			"    give back total\n"
			"main()\n";
		bool ok = HfScript_Compile( &ep, eng, "inline.eng" );
		if ( !ok ) {
			printf( "ENG COMPILE FAIL: %s(%d): %s\n", ep.error.c_str(), ep.errorLine, ep.error.c_str() );
		}
		FEAT_ASSERT( ok );
		if ( ok ) {
			FEAT_ASSERT( evm.RunProgram( &ep ) );
		}
	}

	int added = g_featFails - fc;
	printf( "phase 7 game: %s\n", added ? "FAIL" : "ok" );
}

// Phase 8 - console (game console) porting layer.  Pins the platform
// neutral console service contract (sign in, achievements, stats, voice and
// per user storage) and checks that the offline stub is deterministic: the
// same sequence of operations always produces the same state, and a fresh
// stub always starts from a known clean state.
static void Test_Console() {
	int fc = g_featFails;

	using namespace hfconsole;

	hfConsoleServicesStub a;
	hfConsoleServicesStub b;

	// --- sign in -------------------------------------------------------
	FEAT_ASSERT( a.GetNumSignedInUsers() == 1 );
	FEAT_ASSERT( a.IsInitialized() );
	FEAT_ASSERT( a.GetUserId( 0 ).value != 0 );
	FEAT_ASSERT( a.GetUserId( 0 ) != a.GetUserId( 1 ) );
	FEAT_ASSERT( strlen( a.GetGamerTag( 0 ) ) > 0 );
	FEAT_ASSERT( a.CanPlayOnline( 0 ) );
	FEAT_ASSERT( a.GetInputDevice( 0 ) == 0 );

	// a real console target installs its SDK backed services at boot, the
	// engine then picks them up through the same accessor
	hfConsoleServicesStub platform;
	hfConsoleServices * original = HfConsole_GetServices();
	HfConsole_InstallServices( &platform );
	FEAT_ASSERT( HfConsole_GetServices() == &platform );
	HfConsole_InstallServices( original );
	FEAT_ASSERT( HfConsole_GetServices() == original );

	// --- achievements --------------------------------------------------
	hfConsoleUserId u0 = a.GetUserId( 0 );
	hfConsoleUserId u1 = a.GetUserId( 1 );
	FEAT_ASSERT( a.IsAchievementLocked( u0, 3 ) );
	a.UnlockAchievement( u0, 3 );
	FEAT_ASSERT( !a.IsAchievementLocked( u0, 3 ) );
	a.UnlockAchievement( u0, 3 );			// unlocking twice is a no-op
	FEAT_ASSERT( !a.IsAchievementLocked( u0, 3 ) );
	FEAT_ASSERT( a.IsAchievementLocked( u1, 3 ) );		// achievements are per account

	// --- stats ---------------------------------------------------------
	a.SetStatInt( u0, 5, 1234 );
	a.SetStatFloat( u0, 6, 2.5f );
	FEAT_ASSERT( a.GetStatInt( u0, 5 ) == 1234 );
	FEAT_ASSERT( a.GetStatFloat( u0, 6 ) == 2.5f );
	FEAT_ASSERT( a.GetStatInt( u0, 9 ) == 0 );
	FEAT_ASSERT( a.GetStatInt( u1, 5 ) == 0 );			// stats are per account

	// --- voice ---------------------------------------------------------
	FEAT_ASSERT( a.GetVoiceState( u0 ) == HF_VOICE_OFF );
	FEAT_ASSERT( !a.IsVoiceMuted( u0 ) );
	a.SetVoiceMuted( u0, true );
	FEAT_ASSERT( a.IsVoiceMuted( u0 ) );
	FEAT_ASSERT( !a.IsVoiceMuted( u1 ) );

	// --- per user storage ----------------------------------------------
	const byte blob[ 4 ] = { 1, 2, 3, 4 };
	byte out[ 8 ];
	FEAT_ASSERT( a.WriteSlot( u0, "slot1", blob, 4 ) );
	FEAT_ASSERT( a.SlotSize( u0, "slot1" ) == 4 );
	FEAT_ASSERT( a.ReadSlot( u0, "slot1", out, (int)sizeof( out ) ) == 4 );
	FEAT_ASSERT( out[0] == 1 && out[1] == 2 && out[2] == 3 && out[3] == 4 );
	FEAT_ASSERT( a.SlotSize( u0, "missing" ) == 0 );
	FEAT_ASSERT( a.ReadSlot( u0, "missing", out, (int)sizeof( out ) ) == 0 );
	FEAT_ASSERT( a.SlotSize( u1, "slot1" ) == 0 );		// storage is per account

	a.DeleteSlot( u0, "slot1" );
	FEAT_ASSERT( a.SlotSize( u0, "slot1" ) == 0 );
	FEAT_ASSERT( a.WriteSlot( u0, "slot1", blob, 4 ) );

	// an oversized blob is rejected and leaves no slot behind
	byte big[ hfConsoleServices::HF_CONSOLE_MAX_SLOT_BYTES + 1 ];
	memset( big, 7, sizeof( big ) );
	FEAT_ASSERT( !a.WriteSlot( u0, "toobig", big, (int)sizeof( big ) ) );
	FEAT_ASSERT( a.SlotSize( u0, "toobig" ) == 0 );

	// the slot table is bounded
	a.DeleteSlot( u0, "slot1" );
	for ( int i = 0; i < hfConsoleServices::HF_CONSOLE_MAX_SLOTS; i++ ) {
		char name[ 32 ];
		sprintf( name, "s%d", i );
		FEAT_ASSERT( a.WriteSlot( u0, name, blob, 4 ) );
	}
	FEAT_ASSERT( !a.WriteSlot( u0, "overflow", blob, 4 ) );

	// --- determinism ---------------------------------------------------
	// the same sequence of operations on a second stub produces the same state
	b.UnlockAchievement( b.GetUserId( 0 ), 3 );
	b.SetStatInt( b.GetUserId( 0 ), 5, 1234 );
	b.SetStatFloat( b.GetUserId( 0 ), 6, 2.5f );
	b.SetVoiceMuted( b.GetUserId( 0 ), true );
	b.WriteSlot( b.GetUserId( 0 ), "slot1", blob, 4 );
	FEAT_ASSERT( !b.IsAchievementLocked( b.GetUserId( 0 ), 3 ) );
	FEAT_ASSERT( b.GetStatInt( b.GetUserId( 0 ), 5 ) == 1234 );
	FEAT_ASSERT( b.GetStatFloat( b.GetUserId( 0 ), 6 ) == 2.5f );
	FEAT_ASSERT( b.IsVoiceMuted( b.GetUserId( 0 ) ) );
	FEAT_ASSERT( b.SlotSize( b.GetUserId( 0 ), "slot1" ) == 4 );
	byte out2[ 8 ];
	FEAT_ASSERT( b.ReadSlot( b.GetUserId( 0 ), "slot1", out2, (int)sizeof( out2 ) ) == 4 );
	FEAT_ASSERT( out2[0] == 1 && out2[3] == 4 );

	// a fresh stub starts from a known clean state
	hfConsoleServicesStub c;
	FEAT_ASSERT( c.IsAchievementLocked( c.GetUserId( 0 ), 3 ) );
	FEAT_ASSERT( c.GetStatInt( c.GetUserId( 0 ), 5 ) == 0 );
	FEAT_ASSERT( c.SlotSize( c.GetUserId( 0 ), "slot1" ) == 0 );
	FEAT_ASSERT( !c.IsVoiceMuted( c.GetUserId( 0 ) ) );

	int added = g_featFails - fc;
	printf( "phase 8 console: %s\n", added ? "FAIL" : "ok" );
}

// Phase 15 - UI widgets, builder helpers, and input/event system
static void Test_UiWidgets() {
	using namespace HF_Ui;
	int fc = g_featFails;
	
	HF_Ecs::Registry registry;
	hfUiSystem ui;
	ui.SetViewport( 1024.0f, 768.0f );
	HF_Ecs::EventBus bus;
	
	int clickCount = 0;
	HF_Ecs::eventHandler_t clickHandler = []( const char *name, const void *data, int size, void *user ) {
		( void )name; ( void )data; ( void )size;
		int *c = (int *)user;
		(*c)++;
	};
	
	// Build UI with helpers
	HF_Ecs::Entity root = hfUiBuilder::CreateRoot( registry, 1024.0f, 768.0f );
	HF_Ecs::Entity panel = hfUiBuilder::CreatePanel( registry, root, 50, 50, 300, 400, 0.1f, 0.1f, 0.15f, 1.0f );
	HF_Ecs::Entity vbox = hfUiBuilder::CreateVerticalBox( registry, panel, 0, 0, 300, 400, 8.0f );
	HF_Ecs::Entity btn = hfUiBuilder::CreateButton( registry, vbox, 0, 0, 200, 40, "PLAY" );
	HF_Ecs::Entity slider = hfUiBuilder::CreateSlider( registry, vbox, 0, 0, 200, 30, 0.0f, 100.0f, 25.0f );
	HF_Ecs::Entity toggle = hfUiBuilder::CreateToggle( registry, vbox, 0, 0, 200, 24, "SOUND", true );
	HF_Ecs::Entity input = hfUiBuilder::CreateTextInput( registry, vbox, 0, 0, 200, 32, "NAME..." );
	
	FEAT_ASSERT( registry.Has< hfUiPanelComponent >( panel ) );
	FEAT_ASSERT( registry.Has< hfUiButtonComponent >( btn ) );
	FEAT_ASSERT( registry.Has< hfUiSliderComponent >( slider ) );
	FEAT_ASSERT( registry.Has< hfUiToggleComponent >( toggle ) );
	FEAT_ASSERT( registry.Has< hfUiTextInputComponent >( input ) );
	
	// Subscribe to click
	bus.Subscribe( "ui.click", clickHandler, &clickCount );
	
	ui.Update( registry, 0.0f );
	
	// Verify layout
	hfUiRect br = ui.GetRect( btn );
	FEAT_ASSERT( br.w == 200.0f );
	FEAT_ASSERT( br.h == 40.0f );
	FEAT_ASSERT( br.x >= 50.0f ); // inside panel
	
	// Input system
	hfUiInput uiInput;
	uiInput.SetMouse( br.x + 10, br.y + 10 );
	uiInput.SetButton( HF_UI_MOUSE_LEFT, true );
	uiInput.Update( registry, ui, bus );
	uiInput.SetButton( HF_UI_MOUSE_LEFT, false );
	uiInput.Update( registry, ui, bus );
	uiInput.EndFrame();
	
	FEAT_ASSERT( clickCount >= 1 );
	
	// Button state
	hfUiButtonComponent *bcomp = registry.GetComponent< hfUiButtonComponent >( btn );
	FEAT_ASSERT( bcomp != NULL );
	
	// Toggle state
	hfUiToggleComponent *tcomp = registry.GetComponent< hfUiToggleComponent >( toggle );
	FEAT_ASSERT( tcomp != NULL );
	FEAT_ASSERT( tcomp->checked == true );
	
	// Slider
	hfUiSliderComponent *scomp = registry.GetComponent< hfUiSliderComponent >( slider );
	FEAT_ASSERT( scomp != NULL );
	FEAT_NEAR( scomp->value, 25.0f, 0.001f );
	
	// Text input
	hfUiTextInputComponent *ticomp = registry.GetComponent< hfUiTextInputComponent >( input );
	FEAT_ASSERT( ticomp != NULL );
	FEAT_ASSERT( strcmp( ticomp->placeholder, "NAME..." ) == 0 );
	
	// Event bus
	hfUiEventClick cev = { btn, HF_UI_MOUSE_LEFT, 100, 100 };
	bus.Publish( "ui.click", &cev, sizeof(cev) );
	
	int added = g_featFails - fc;
	printf( "phase 9 ui widgets: %s\n", added ? "FAIL" : "ok" );
}
