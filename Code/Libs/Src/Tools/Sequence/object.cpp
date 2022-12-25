#include "stdafx.h"
#include <stdio.h>

#include "object.h"
#include "character.h"
#include "mesh.h"
#include "sequence.h"
#include "icamera.h"
#include <RPUL/PrimitiveBuilder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "gametypes.h"

extern	SequenceApp theApp;

int num_object_polys_to_render = 1;

#define DEL(ptr)		\
	if (ptr != NULL) {	\
		delete ptr;		\
	}

#define DELA(ptr)		\
	if (ptr != NULL) {	\
		delete [] ptr;	\
	}

#define DELC(ptr)		\
	if (ptr != NULL) {	\
		delete ptr;		\
		ptr = NULL;		\
	}

#define DELCA(ptr)		\
	if (ptr != NULL) {	\
		delete [] ptr;	\
		ptr = NULL;		\
	}

char* ObjectTypeNames[] = {
	"OT_BODY",
	"OT_BOMB",
	"OT_STATIC_GEOMETRY",
	"OT_CHARACTER",
	"OT_GAME_OBJECT",
	"OT_LIGHT",
	"OT_EXPLOSION",
	"OT_GUN"
};

char* ObjectTypePrettyNames[] = {
	"Phys Object",
	"Bomb",
	"Static",
	"Character",
	"Game Object",
	"Light",
	"Explosion",
	"Gun"
};

//

ObjectType BaseObject::get_type(void) const
{
	ObjectType result = (archetype) ? archetype->type : OT_GAME_OBJECT;
	return result;
}

//
// GameObject methods
//

GameObject::GameObject() 
{
	index = INVALID_INSTANCE_INDEX;
	current_state = 1;
	name = NULL;
	radius = 1.0f;
	editor_hidden = false;
	editor_frozen = false;
	designer_created = false;

	parent_hp_name = NULL;
	hp_name = NULL;

	archetype = NULL;

	m_transform.set_identity();
}

//

GameObject::~GameObject(void) 
{
	if (archetype != NULL) 
	{
		archetype->delete_instance(this);
	}

	if (index != INVALID_INSTANCE_INDEX) 
	{
		theApp.m_ENG->destroy_instance(index);
	}

	DELA(name);
	DELA(parent_hp_name);
	DELA(hp_name);
}

//

struct hp_callback_data {
	ICamera* the_camera;
	ARCHETYPE_INDEX arch_idx;

	BaseObject* obj;
};

void __cdecl render_hardpoint_callback(const char* script_name, void* data) {
	if (strcmp(script_name, "ground") == 0) {
		return;
	}

	hp_callback_data* the_data = (hp_callback_data*)data;

	HardpointInfo hp_info;
	theApp.m_HARDPOINT->retrieve_hardpoint_info(the_data->arch_idx, script_name, hp_info);

	// Draw a box here
	Vector pos = hp_info.point;
	//Vector pos = the_data->obj->get_transform(the_data->obj->get_index()).rotate_translate(hp_info.point);

	RPVertex pts[3];
	memset(pts, 0, sizeof(RPVertex) * 3);
	pts[0].pos = pos + hp_info.orientation.get_j() * 0.15f;
	pts[1].pos = pos - hp_info.orientation.get_j() * 0.15f + hp_info.orientation.get_i() * 0.3f;
	pts[2].pos = pos - hp_info.orientation.get_j() * 0.15f - hp_info.orientation.get_i() * 0.3f;

	//pts[0].pos = pos + the_data->the_camera->get_transform().get_orientation().get_j() * 0.15f;
	//pts[1].pos = pos - the_data->the_camera->get_transform().get_orientation().get_j() * 0.15f + the_data->the_camera->get_transform().get_orientation().get_i() * 0.3f;
	//pts[2].pos = pos - the_data->the_camera->get_transform().get_orientation().get_j() * 0.15f - the_data->the_camera->get_transform().get_orientation().get_i() * 0.3f;

/*	if ((active_dragging_obj == the_data->obj) && (strcmp(script_name, active_dragging_obj_hp_name) == 0)) {
		pts[0].g = 255;
		pts[1].g = 255;
		pts[2].g = 255;
	} else if ((active_drag_to_obj == the_data->obj) && (strcmp(script_name, active_drag_to_obj_hp_name) == 0)) {
		pts[0].r = 255;
		pts[1].r = 255;
		pts[2].r = 255;
	} else*/ {
		pts[0].b = 255;
		pts[1].b = 255;
		pts[2].b = 255;
	}

	GENRESULT result = theApp.m_BATCH->draw_primitive(D3DPT_TRIANGLELIST, D3DVT_RPVERTEX, pts, 3, 0);
}

void GameObject::Render(ICamera* the_camera, U32 flags)
{
	bool is_a_child = (theApp.m_MODEL->get_parent(get_index()) != INVALID_INSTANCE_INDEX);
	bool visible = false;

	if (!is_a_child)// || show_hardpoints) 
	{
		// Children do not need to render themselves

		if (flags == RF_OUTLINE) 
		{
			theApp.SetNoTexture();
			theApp.m_BATCH->set_render_state(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		} else {
			theApp.SetDefaultTextureBlend();
			theApp.m_BATCH->set_render_state(D3DRS_FILLMODE, D3DFILL_SOLID);
		}

		vis_state vs = theApp.m_ENG->render_instance(the_camera, index, flags);
		visible = ((vs == VS_PARTIALLY_VISIBLE) || (vs == VS_FULLY_VISIBLE));
	}

	if(0)//show_hardpoints) 
	{
		theApp.SetNoTexture();
		theApp.m_BATCH->set_render_state(D3DRS_FILLMODE, D3DFILL_SOLID);
		theApp.m_BATCH->set_render_state(D3DRS_ZENABLE, FALSE);

		// We need to do this in case this is a parent object and the children changed the modelview matrix
		Transform camx_inv = the_camera->get_inverse_transform();
		theApp.m_BATCH->set_modelview(camx_inv * get_transform(index));

		hp_callback_data data;
		data.the_camera = the_camera;
		data.arch_idx = theApp.m_ENG->get_archetype(index);
		data.obj = this;

		theApp.m_HARDPOINT->enumerate_hardpoints(render_hardpoint_callback, data.arch_idx, &data);
		theApp.m_ENG->release_archetype(data.arch_idx);

		theApp.m_BATCH->set_render_state(D3DRS_ZENABLE, TRUE);

		/*JOINT_INDEX ji = theApp.m_MODEL->traverse_joints(index);
		while (ji != INVALID_JOINT_INDEX) {
			// Get the joint type
			JointType type = theApp.m_MODEL->get_joint_type(ji);

			int size = theApp.m_MODEL->get_joint_data_size(type);
			float* joint_data = new float[size];
			theApp.m_MODEL->get_joint_data(ji, joint_data);

			const Joint* joint_info = theApp.m_MODEL->get_joint(ji);
			Vector pos = get_position() + joint_info->rel_position;

			// Draw a box here
			RPVertex pts[3];
			memset(pts, 0, sizeof(RPVertex) * 3);
			pts[0].pos = pos;
			pts[1].pos = pos + the_camera->get_transform().get_orientation().get_i() * 2.0f;
			pts[2].pos = pos + the_camera->get_transform().get_orientation().get_j() * 2.0f;

			pts[0].b = 255;
			pts[1].b = 255;
			pts[2].b = 255;
			GENRESULT result = theApp.m_BATCH->draw_primitive(D3DPT_TRIANGLELIST, D3DVT_RPVERTEX, pts, 3, 0);

			DELA(joint_data);

			ji = theApp.m_MODEL->traverse_joints(index, ji);
		}*/
	}

	//if (flags == RF_OUTLINE) {
		theApp.SetDefaultTextureBlend();
	//}

#if 0
// SHADOW

	if (is_a_child || !visible)
	{
	// object didn't render itself, needs to make sure modelview is correct.
		Transform T(get_orientation(), get_position());

		Transform M = the_camera->get_inverse_transform() * T;
		theApp.m_BATCH->set_modelview(M);
	}

	struct Mesh * mesh = theApp.m_REND->get_instance_mesh(index);
	if (mesh && archetype->type != OT_STATIC_GEOMETRY)
	{
		float ty =0;// The_level.GetTerrainHeight(pos.x, pos.z);
		float dty = ty + 0.1;

		theApp.SetNoTexture();
		theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,	TRUE);
		theApp.m_BATCH->set_render_state(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA);
		theApp.m_BATCH->set_render_state(D3DRS_DESTBLEND,		D3DBLEND_INVSRCALPHA);

		PrimitiveBuilder pb(theApp.m_BATCH);
		pb.Color4ub(0, 0, 0, 128);
		pb.Begin(PB_TRIANGLES);

		Link<GameLight *> * node = The_level.m_light_list.get_head();
		if (node)
		{
			Vector L = node->obj->get_position();

		// transform L, dL into object space.
			L = (L - pos) * R;
			Vector dL = pos - L;

			Vector Np(0, 1, 0);
			Vector point_on_plane(pos.x, dty, pos.z);

		// rotate Np to object frame.
			Np = Np * R;
 			point_on_plane = (point_on_plane - pos) * R;

			float D = -dot_product(Np, point_on_plane);

			float light_dot = dot_product(L, Np);
			float alpha = light_dot + D;
			Vector vert[3];

		// TODO: pre-project all vertices, look up results. Lots of sharing in objects.
		// But also lots of faces & verts that don't participate in shadow.

			for (int f = 0; f < mesh->face_cnt; f++)
			{
				Vector * v0 = mesh->get_face_vertex(f, 0);
				Vector * v1 = mesh->get_face_vertex(f, 1);
				Vector * v2 = mesh->get_face_vertex(f, 2);

				float d0 = dot_product(*v0, Np);
				float d1 = dot_product(*v1, Np);
				float d2 = dot_product(*v2, Np);
				if (d0 < light_dot && d1 < light_dot && d2 < light_dot)
				{
					if (((d0 + D) >= 0) &&
						((d1 + D) >= 0) &&
						((d2 + D) >= 0))
					{
						Vector * N = mesh->get_face_normal(f);
						if (dot_product(dL, *N) < 0)
						{
							Vector dir = *v0 - L;
							float dd = dot_product(dir, Np);
							float t = -alpha / dd;
							Vector p = L + t * dir;
							pb.Vertex3f(p.x, p.y, p.z);

							dir = *v1 - L;
							dd = dot_product(dir, Np);
							t = -alpha / dd;
							p = L + t * dir;
							pb.Vertex3f(p.x, p.y, p.z);

							dir = *v2 - L;
							dd = dot_product(dir, Np);
							t = -alpha / dd;
							p = L + t * dir;
							pb.Vertex3f(p.x, p.y, p.z);
						}
					}
				}
			}

			pb.End();

			theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
		}
	}
#endif

}

bool GameObject::Update(float secs) {
	// FK: Everything here is a HACK

	// Put in only to see guard tower elevators move.
	// Move the joints
	JOINT_INDEX ji = theApp.m_MODEL->traverse_joints(index);
	while (ji != INVALID_JOINT_INDEX) {
		// Get the joint type
		JointType type = theApp.m_MODEL->get_joint_type(ji);
		switch (type) {
			case JT_PRISMATIC:
			{
				int size = theApp.m_MODEL->get_joint_data_size(JT_PRISMATIC);
				const Joint* joint_info = theApp.m_MODEL->get_joint(ji);

				float* joint_data = new float[size];
				theApp.m_MODEL->get_joint_data(ji, joint_data);

				joint_data[0] += (secs * current_state);

				if ((joint_data[0] >= joint_info->max0) && (current_state == 1)) {
					current_state = -1;
				} else if ((joint_data[0] <= joint_info->min0) && (current_state == -1)) {
					current_state = 1;
				}

				theApp.m_MODEL->set_joint_data(ji, joint_data);
				DELA(joint_data);
				break;
			}

			default:
				;
				break;
		};

		ji = theApp.m_MODEL->traverse_joints(index, ji);
	}

	//theApp.m_MODEL->update_tree(index);

	return true;
}

void GameObject::GetUserData()
{
	assert(strlen(name) <= 79);

	GameObjectData obj_data;
	strcpy(obj_data.instance_name, name);
	obj_data.index = index;
	obj_data.position = m_transform.get_position();
	obj_data.orientation = m_transform.get_orientation();
	obj_data.radius = radius;
	obj_data.current_state = current_state;

/*	theApp.GetUserData("GameObjectData", "testing", &obj_data, sizeof(GameObjectData));
	//theApp.GetUserData("GameObjectData", "testing", (GameObjectData*)this, sizeof(GameObjectData));

	if ((obj_data.instance_name[0] != '\0') && (strcmp(name, obj_data.instance_name) != 0)) {
		// TODO: Make sure if the name has changed that it's not the name of another object
		DELA(name);
		name = new char[strlen(obj_data.instance_name) + 1];
		strcpy(name, obj_data.instance_name);
	}*/
}

void COMAPI GameObject::initialize_instance (INSTANCE_INDEX _index)
{
	index = _index;
	destroyed = false;

	theApp.m_ENG->set_user_data(index, (S32) this);
	theApp.m_PHY->set_dynamic(index, DS_NONDYNAMIC);
}

void COMAPI GameObject::destroy_instance (INSTANCE_INDEX _index)
{
	index = INVALID_INSTANCE_INDEX;
	destroyed = true;
}

bool GameObject::was_destroyed(void) const
{
	return destroyed;
}

const Vector& GameObject::get_position(void) const
{
	return m_transform.get_position();
}

void GameObject::set_position(const Vector & new_pos)
{
	m_transform.set_position(new_pos);
	//theApp.m_MODEL->update_tree(index);
}

const Matrix& GameObject::get_orientation(void) const
{
	return m_transform.get_orientation();
}

void GameObject::set_orientation(const Matrix& new_orient)
{
	m_transform.set_orientation(new_orient);
	//theApp.m_MODEL->update_tree(index);
}

void COMAPI GameObject::set_position(INSTANCE_INDEX index, const Vector & position)
{
	m_transform.set_position(position);
	//theApp.m_MODEL->update_tree(index);
}

const Vector & COMAPI GameObject::get_position(INSTANCE_INDEX index) const
{
	return m_transform.get_position();
}

void COMAPI GameObject::set_orientation(INSTANCE_INDEX index, const Matrix & orientation)
{
	m_transform.set_orientation(orientation);
	//theApp.m_MODEL->update_tree(index);
}

const Matrix & COMAPI GameObject::get_orientation(INSTANCE_INDEX index) const
{
	return m_transform.get_orientation();
}

void COMAPI GameObject::set_transform(INSTANCE_INDEX index, const Transform & transform)
{
	m_transform = transform;
	//theApp.m_MODEL->update_tree(index);
}

const Transform & COMAPI GameObject::get_transform(INSTANCE_INDEX index) const
{
	return m_transform;
}

static Vector zero_vector(0, 0, 0);

const Vector & COMAPI GameObject::get_velocity (INSTANCE_INDEX object) const
{
	return zero_vector;
}

const Vector & COMAPI GameObject::get_angular_velocity (INSTANCE_INDEX object) const
{
	return zero_vector;
}

const BaseExtent* GameObject::get_extent() const {
	const BaseExtent* obj_extent = NULL;
	theApp.m_PHY->get_extent(&obj_extent, index);
	return obj_extent;
}

bool GameObject::save_instance(IFileSystem* file) {
	// Save our instance data
	DAFILEDESC desc;
	desc.dwDesiredAccess = GENERIC_WRITE;
	desc.dwCreationDistribution = CREATE_ALWAYS;
	desc.dwShareMode = 0;

	HANDLE handle;
	DWORD bytes_written = 0;

	desc.lpFileName = "name";
	handle = file->OpenChild(&desc);
	file->WriteFile(handle, name, strlen(name), &bytes_written);
	file->CloseHandle(handle);

	desc.lpFileName = "pos";
	handle = file->OpenChild(&desc);
	file->WriteFile(handle, &m_transform.translation, sizeof(Vector), &bytes_written);
	file->CloseHandle(handle);

	desc.lpFileName = "R";
	handle = file->OpenChild(&desc);
	file->WriteFile(handle, &m_transform, sizeof(Matrix), &bytes_written);
	file->CloseHandle(handle);

	desc.lpFileName = "radius";
	handle = file->OpenChild(&desc);
	file->WriteFile(handle, &radius, sizeof(float), &bytes_written);
	file->CloseHandle(handle);

	desc.lpFileName = "current_state";
	handle = file->OpenChild(&desc);
	file->WriteFile(handle, &current_state, sizeof(int), &bytes_written);
	file->CloseHandle(handle);

	return true;
}

bool GameObject::load_instance(IFileSystem* file) {
	DAFILEDESC desc;
	desc.dwDesiredAccess = GENERIC_READ;
	desc.dwShareMode = 0;

	HANDLE handle;
	DWORD bytes_read = 0;

	desc.lpFileName = "name";
	handle = file->OpenChild(&desc);

	int length = file->GetFileSize(handle);
	DELA(name);
	name = new char[length + 1];

	file->ReadFile(handle, name, length, &bytes_read);
	name[length] = '\0';
	file->CloseHandle(handle);

	// Check here if the name is the name of some other object in the world..ask for a new name

	desc.lpFileName = "pos";
	handle = file->OpenChild(&desc);
	file->ReadFile(handle, &m_transform.translation, sizeof(Vector), &bytes_read);
	file->CloseHandle(handle);

	desc.lpFileName = "R";
	handle = file->OpenChild(&desc);
	file->ReadFile(handle, &m_transform, sizeof(Matrix), &bytes_read);
	file->CloseHandle(handle);

	desc.lpFileName = "radius";
	handle = file->OpenChild(&desc);
	file->ReadFile(handle, &radius, sizeof(float), &bytes_read);
	file->CloseHandle(handle);

	desc.lpFileName = "current_state";
	handle = file->OpenChild(&desc);
	file->ReadFile(handle, &current_state, sizeof(int), &bytes_read);
	file->CloseHandle(handle);
	return true;
}

ObjectArchetype::ObjectArchetype(const char* name) {
	type = OT_INVALID_OBJ_TYPE;

	m_name = new char[strlen(name) + 1];
	strcpy(m_name, name);

	ref_count = 0;
	delete_if_no_ref = true;

	cur_instance_num = 0;
}

ObjectArchetype::~ObjectArchetype() {
	if (ref_count != 0) {
		char buffer[80];
		sprintf(buffer, "BF Archetype %s: %d dangling references\n", m_name, ref_count);
		OutputDebugString(buffer);
	}

	DELA(m_name);
}

void ObjectArchetype::add_ref() {
	ref_count++;
}

void ObjectArchetype::remove_ref() {
	ref_count--;

	assert(ref_count >= 0);

	if ((delete_if_no_ref) && (ref_count <= 0)) {
//		The_level.m_obj_archetypes.remove(this);
		DEL(this);
	}
}

//
// class Static Geometry Archetype
//
StaticGeomArchetype::StaticGeomArchetype(const char* name) : ObjectArchetype(name) {
	type = OT_STATIC_GEOMETRY;

	mesh_name = NULL;
	arch_idx = INVALID_ARCHETYPE_INDEX;

	// Load a section from the building.ini file
	char buffer[_MAX_PATH];
	sprintf(buffer, "building.ini");

	if (theApp.m_PROF->Initialize(buffer) == GR_OK) {
		HANDLE h = theApp.m_PROF->CreateSection(name);
		if (h != NULL) {
			if (theApp.m_PROF->ReadKeyValue(h, "model", buffer, _MAX_PATH)) {
				mesh_name = new char[strlen(buffer) + 1];
				strcpy(mesh_name, buffer);
			}
			if (theApp.m_PROF->ReadKeyValue(h, "stay loaded", buffer, _MAX_PATH)) {
				if ((buffer[0] == 'y') || (buffer[0] == 'Y')) {
					delete_if_no_ref = false;
				}
			}
			theApp.m_PROF->CloseSection(h);
		}
	}
}

StaticGeomArchetype::~StaticGeomArchetype() {
	DELA(mesh_name);

	if (arch_idx != INVALID_ARCHETYPE_INDEX) {
		theApp.m_ENG->release_archetype(arch_idx);
	}
}

BaseObject* StaticGeomArchetype::create_instance() {
	char buffer[_MAX_PATH];

	// Make a new object
	GameObject* new_building = new GameObject();

	if (new_building == NULL) {
		OutputDebugString("StaticGeomArchetype: Object creation failed!\n");
	} else {
		// Make sure we have the model loaded
		if (arch_idx == INVALID_ARCHETYPE_INDEX) {
			sprintf(buffer, "data\\objects\\%s", mesh_name);
			IFileSystem* file;
			DAFILEDESC desc(buffer);

			if (theApp.m_DACOM->CreateInstance(&desc, (void**) &file) == GR_OK) {
				// Load any textures out of the file
				theApp.m_TEXLIB->load_library(file, NULL);

				// Load the archetype
				arch_idx = theApp.m_ENG->create_archetype(mesh_name, file);
				file->Release();
			}

			if (arch_idx == INVALID_ARCHETYPE_INDEX) {
				sprintf(buffer, "StaticGeomArchetype: Couldn't create instance of %s.\n", mesh_name);
				OutputDebugString(buffer);

				add_ref();
				DEL(new_building);

				DEL(this);
				return NULL;
			}
		}

		int index = theApp.m_ENG->create_instance2(arch_idx, new_building);

		if (index != INVALID_INSTANCE_INDEX) {
			Transform spot;
			spot.set_identity();
			new_building->set_position(spot.get_position());
			new_building->set_orientation(spot.get_orientation());

			new_building->archetype = this;

			sprintf(buffer, "%s_%d", m_name, cur_instance_num);
			new_building->name = new char[strlen(buffer) + 1];
			strcpy(new_building->name, buffer);
			cur_instance_num++;

			add_ref();

		} else {
			DELC(new_building);
			OutputDebugString("Failed to create engine instance!\n");
		}
	}

	return new_building;
}

void StaticGeomArchetype::delete_instance(BaseObject* obj) {
	remove_ref();
}

//
// class Game Object Archetype
//
GameObjectArchetype::GameObjectArchetype(const char* name) : ObjectArchetype(name) {
	type = OT_GAME_OBJECT;

	mesh_name = NULL;
	arch_idx = INVALID_ARCHETYPE_INDEX;

	// Load a section from the weapon.ini file
	char buffer[_MAX_PATH];
	sprintf(buffer, "gameobjs.ini");

	if (theApp.m_PROF->Initialize(buffer) == GR_OK) {
		HANDLE h = theApp.m_PROF->CreateSection(name);
		if (h != NULL) {
			if (theApp.m_PROF->ReadKeyValue(h, "model", buffer, _MAX_PATH)) {
				mesh_name = new char[strlen(buffer) + 1];
				strcpy(mesh_name, buffer);
			}
			if (theApp.m_PROF->ReadKeyValue(h, "stay loaded", buffer, _MAX_PATH)) {
				if ((buffer[0] == 'y') || (buffer[0] == 'Y')) {
					delete_if_no_ref = false;
				}
			}
			theApp.m_PROF->CloseSection(h);
		}
	}
}

GameObjectArchetype::~GameObjectArchetype() {
	DELA(mesh_name);

	if (arch_idx != INVALID_ARCHETYPE_INDEX) {
		theApp.m_ENG->release_archetype(arch_idx);
	}
}

BaseObject* GameObjectArchetype::create_instance() {
	char buffer[_MAX_PATH];

	// Make a new object
	GameObject* new_obj = new GameObject();

	if (new_obj == NULL) {
		OutputDebugString("GameObjectArchetype: Object creation failed!\n");
	} else {
		// Make sure we have the model loaded
		if (arch_idx == INVALID_ARCHETYPE_INDEX) {
			sprintf(buffer, "data\\objects\\%s", mesh_name);
			IFileSystem* file;
			DAFILEDESC desc(buffer);

			if (theApp.m_DACOM->CreateInstance(&desc, (void**) &file) == GR_OK) {
				// Load any textures out of the file
				theApp.m_TEXLIB->load_library(file, NULL);

				// Load the archetype
				arch_idx = theApp.m_ENG->create_archetype(mesh_name, file);
				file->Release();
			}

			if (arch_idx == INVALID_ARCHETYPE_INDEX) {
				sprintf(buffer, "GameObjectArchetype: Couldn't create instance of %s.\n", mesh_name);
				OutputDebugString(buffer);

				add_ref();
				DEL(new_obj);

				DEL(this);
				return NULL;
			}
		}

		int index = theApp.m_ENG->create_instance2(arch_idx, new_obj);

		if (index != INVALID_INSTANCE_INDEX) {
			Transform spot;
			spot.set_identity();
			new_obj->set_position(spot.get_position());
			new_obj->set_orientation(spot.get_orientation());

			new_obj->archetype = this;
		} else {
			DELC(new_obj);
			OutputDebugString("Failed to create engine instance!\n");
		}

		sprintf(buffer, "%s_%d", m_name, cur_instance_num);
		new_obj->name = new char[strlen(buffer) + 1];
		strcpy(new_obj->name, buffer);
		cur_instance_num++;

		add_ref();
	}

	return new_obj;
}

void GameObjectArchetype::delete_instance(BaseObject* obj) {
	remove_ref();
}

