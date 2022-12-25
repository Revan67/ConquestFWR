#include "stdafx.h"
#include <stdio.h>
#include <ChannelEventTypes.h>

#include "character.h"
#include "gun.h"
#include <iprofileparser.h>
#include "sequence.h"

extern	SequenceApp theApp;


inline float __cdecl Degrees2Radians(float deg) {
	return deg * PI / 180.0;
}

static	COLORREF	RandomRGB(void)
{
	return	RGB(
		(rand() / (RAND_MAX / 255)),
		(rand() / (RAND_MAX / 255)),
		(rand() / (RAND_MAX / 255)));
}

static	int	GetBranchDepth(MotionSequence *ms, int depth)
{
	if(ms)
	{
		if(!ms->bVisited)
		{
			ms->bVisited	=TRUE;
			return	GetBranchDepth(ms->end, depth + 1);
		}
	}

	return	depth - 1;
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Distinction between command and sequences (MT_*). Commands are higher level (toggle weapon, turn left, etc.)
// that are then mapped to sequences based on character's current state, etc.
//
// e.g. character knows whether weapon is ready or not, so maps TOGGLE_WEAPON_READY to either MT_DRAW_WEAPON or
// MT_HOLSTER_WEAPON. e.g. TURN_RIGHT maps to MT_TURN_IN_PLACE if character is standing still, just adjusts yaw
// if character is walking. etc. etc.
//

//
// class Character Archetype
//

BOOL32 __stdcall CharacterArchetype::enum_callback(struct IProfileParser * parser, const C8 * sectionName, void *context)
{
	CharacterArchetype * arch = (CharacterArchetype *) context;

	for (int i = 0; i < MT_NUM_MOTION_TYPES; i++)
	{
		if (strcmp(sectionName, MotionTypeNames[i]) == 0)
		{
			HANDLE h = parser->CreateSection(sectionName);
			if (h != NULL)
			{
				char buffer[80];
				for (int j = 0; j < MT_NUM_MOTION_TYPES; j++)
				{
					if (parser->ReadKeyValue(h, MotionTypeNames[j], buffer, 80))
					{
						for (int k = 0; k < arch->num_sequences; k++)
						{
							if (strcmp(buffer, arch->sequences[k].name) == 0)
							{
								arch->seq_grid[i][j] = arch->sequences + k;
								break;
							}
						}
					}
				}

				parser->CloseSection(h);
			}

			break;
		}
	}
	
	return TRUE;
}

//

CharacterArchetype::CharacterArchetype(const char* name) : ObjectArchetype(name)
{
	HANDLE			h;
	int				i, j;//, MaxXExtent;

	type		=OT_CHARACTER;
	GDat		=NULL;
	mesh_name = NULL;
	anim_name = NULL;
	arch_idx = INVALID_ARCHETYPE_INDEX;

	anim_arch = INVALID_SCRIPT_SET_ARCH;
	num_scripts = 0;
	script_names = NULL;

	num_sequences = 0;
	sequences = NULL;

	edges = NULL;
	num_edges = 0;

	num_attachments = 0;
	attachments = NULL;

	num_transfers = 0;
	transfers = NULL;

// Load a section from the weapon.ini file
	char buffer[_MAX_PATH];
	sprintf(buffer, "data\\characters\\%s.ini", name);

	if (theApp.m_PROF->Initialize(buffer) == GR_OK) 
	{
	//
	// [Object] section - mesh, animations, misc. parameters.
	//
		h = theApp.m_PROF->CreateSection("Object");
		if (h != NULL) 
		{
			if (theApp.m_PROF->ReadKeyValue(h, "mesh", buffer, _MAX_PATH)) 
			{
				mesh_name = new char[strlen(buffer) + 1];
				strcpy(mesh_name, buffer);
			}

			if (theApp.m_PROF->ReadKeyValue(h, "anim", buffer, _MAX_PATH)) 
			{
				anim_name = new char[strlen(buffer) + 1];
				strcpy(anim_name, buffer);
			}

			if (theApp.m_PROF->ReadKeyValue(h, "turnrate", buffer, _MAX_PATH))
			{
				turn_rate = Degrees2Radians(atof(buffer));
			}
			else
			{
				turn_rate = Degrees2Radians(45);
			}

			theApp.m_PROF->CloseSection(h);
		}

	//
	// [Attachments] section.
	//
		h = theApp.m_PROF->CreateSection("Attachments");
		if (h != NULL)
		{
			int line = 0;
			num_attachments = 0;
		// MAX 32 attachments...
			char buffer[32][80];
			int n;
			do
			{
				n = theApp.m_PROF->ReadProfileLine(h, line++, buffer[num_attachments], 80);

				if (n > 1)
				{
					num_attachments++;
				}

			} while (n);

			theApp.m_PROF->CloseSection(h);

			if (num_attachments)
			{
				attachments = new AttachmentArchetype[num_attachments];

				AttachmentArchetype * a = attachments;
				for (i = 0; i < num_attachments; i++, a++)
				{
					a->name = strdup(buffer[i]);
				}		

			// now go read each attachment section.
				char object_name[80][32], parent_hp_name[80][32], child_hp_name[80][32];
				a = attachments;
				for (i = 0; i < num_attachments; i++, a++)
				{
					h = theApp.m_PROF->CreateSection(a->name);
					if (h == NULL)
					{
						char temp[80];
						sprintf(temp, "Unable to create section [%s] in character '%s'.\n", a->name, name);
						GENERAL_ERROR(temp);
					}
					else
					{
						if (theApp.m_PROF->ReadKeyValue(h, "active", buffer[0], 80))
						{
							a->active = (stricmp(buffer[0], "true") == 0);
						}
						else
						{
							a->active = false;
						}

						theApp.m_PROF->ReadKeyValue(h, "child_object",	object_name[i],		80);
						theApp.m_PROF->ReadKeyValue(h, "parent",			parent_hp_name[i],	80);
						theApp.m_PROF->ReadKeyValue(h, "child",			child_hp_name[i],	80);

						if (theApp.m_PROF->ReadKeyValue(h, "select", buffer[0], 80))
						{
							a->select = (stricmp(buffer[0], "true") == 0);
						}
						else
						{
							a->select = false;
						}

						theApp.m_PROF->CloseSection(h);
					}
				}

				a = attachments; 
				for (i = 0; i < num_attachments; i++, a++)
				{
				// HACK HACK HACK hard-coded type. Need a good way to look things up by name only.
					a->child_object	= strdup(object_name[i]);
					a->parent_hp	= strdup(parent_hp_name[i]);
					a->child_hp		= strdup(child_hp_name[i]);
				}
			}

		// BAD SOLUTION. READ EVERYTHING FROM ONE FILE BEFORE DOING ANYTHING WITH IT THAT 
		// INVOLVES OPENING ANOTHER FILE.
		// GetObjectType() screwed the profile parser, point it back to the right file.
			sprintf(buffer[0], "data\\characters\\%s.ini", name);
			theApp.m_PROF->Initialize(buffer[0]);
		}
	
	//
	// [Transfers] - connection/disconnection of child objects.
	//
		h = theApp.m_PROF->CreateSection("Transfers");
		if (h != NULL)
		{
			int line = 0;
			num_transfers = 0;
		// MAX 32 transfers...
			char buffer[32][80];
			int n;
			do
			{
				n = theApp.m_PROF->ReadProfileLine(h, line++, buffer[num_transfers], 80);

				if (n > 1)
				{
					num_transfers++;
				}

			} while (n);

			theApp.m_PROF->CloseSection(h);

			if (num_transfers)
			{
				transfers = new Transfer[num_transfers];

				Transfer * t = transfers;
				for (i = 0; i < num_transfers; i++, t++)
				{
					t->name = strdup(buffer[i]);

					h = theApp.m_PROF->CreateSection(t->name);
					if (h == NULL)
					{
						char temp[80];
						sprintf(temp, "Unable to create section [%s] in character '%s'.\n", t->name, name);
						GENERAL_ERROR(temp);
					}
					else
					{
						if (theApp.m_PROF->ReadKeyValue(h, "event",	buffer[0], 80))
						{
							t->trigger = strdup(buffer[0]);
						}
						if (theApp.m_PROF->ReadKeyValue(h, "source",	buffer[0], 80))
						{
							AttachmentArchetype * a = attachments;
							for (j = 0; j < num_attachments; j++, a++)
							{
								if (strcmp(buffer[0], a->name) == 0)
								{
									t->src = j;
									break;
								}
							}
						}
						if (theApp.m_PROF->ReadKeyValue(h, "dest",	buffer[0], 80))
						{
							t->dst = -1;

							AttachmentArchetype * a = attachments;
							for (j = 0; j < num_attachments; j++, a++)
							{
								if (strcmp(buffer[0], a->name) == 0)
								{
									t->dst = j;
									break;
								}
							}
						}
						else
						{
							t->dst = -1;
						}

						theApp.m_PROF->CloseSection(h);
					}
				}
			}
		}

	//
	// [Sequences] - motion sequences.
	//
		h = theApp.m_PROF->CreateSection("Sequences");
		if (h != NULL)
		{
			int line = 0;
			int ns = 0;
		// MAX 100 sequences...
			char buffer[100][80];
			int n;
			do
			{
				n = theApp.m_PROF->ReadProfileLine(h, line++, buffer[ns], 80);

				if (n > 1)
				{
					ns++;
				}

			} while (n);

			num_sequences			=ns;
			sequences				=new MotionSequence[num_sequences];
			GDat					=new GraphUIData;
			GDat->num_sequences		=ns;
			GDat->BranchY			=new int[num_sequences];
			GDat->NodeBoxes			=new NodeBox[num_sequences];
			GDat->ConnectionCount	=new int[num_sequences];
			GDat->ClusterNum		=new int[num_sequences];
			GDat->BranchDepths		=new int[num_sequences];
			GDat->pSequences		=&sequences;
			GDat->ClusterBoxes		=NULL;

			memset(GDat->ConnectionCount, 0, num_sequences * sizeof(int));

			MotionSequence * ms = sequences;
			for (i = 0; i < num_sequences; i++, ms++)
			{
				ms->name	=new char[80];
				strncpy((char *)ms->name, buffer[i], 80);
			}

			theApp.m_PROF->CloseSection(h);
		}

		MotionSequence * ms = sequences;
		for (i = 0; i < num_sequences; i++, ms++)
		{
			HANDLE h = theApp.m_PROF->CreateSection(ms->name);
			if (h == NULL)
			{
				char temp[80];
				sprintf(temp, "Unable to create section [%s] in character '%s'.\n", ms->name, name);
				GENERAL_ERROR(temp);
			}
			else
			{
				char buffer[80];

				if (theApp.m_PROF->ReadKeyValue(h, "type", buffer, 80))
				{
					for (int t = 0; t < MT_NUM_MOTION_TYPES; t++)
					{
						if (strcmp(buffer, MotionTypeNames[t]) == 0)
						{
							ms->type = (MotionType) t;
							break;
						}
					}

					if (t == MT_NUM_MOTION_TYPES)
					{
						char temp[80];
						sprintf(temp, "Unable to recognize type of sequence %s, file %s.\n", ms->name, name);
						GENERAL_ERROR(temp);
					}
				}
				else
				{
					char temp[80];
					sprintf(temp, "No 'type' key in sequence %s, file %s.\n", ms->name, name);
					GENERAL_ERROR(temp);
				}

				ms->script	=new char[80];
				if (theApp.m_PROF->ReadKeyValue(h, "script", buffer, 80))
				{
					if (stricmp(buffer, "null") == 0)
					{
						ms->script = NULL;
					}
					else
					{
						strncpy((char *)ms->script, buffer, 80);
					}
				}
				else
				{
					j = 0;
					char temp[80];
					sprintf(temp, "wait%d", j);

					char wait_events[16][32];
					char wait_scripts[16][64];

					while (theApp.m_PROF->ReadKeyValue(h, temp, buffer, 80))
					{
						strcpy(wait_events[j], buffer);

						sprintf(temp, "target%d", j);
						if (theApp.m_PROF->ReadKeyValue(h, temp, buffer, 80))
						{
							strcpy(wait_scripts[j], buffer);
						}
						else
						{
							GENERAL_ERROR("No target script associated with wait event in character INI.\n");
						}

						j++;
						sprintf(temp, "wait%d", j);
					}

					ms->num_wait_states = j;
					ms->wait_events = new char *[j];
					ms->wait_targets = new char *[j];

					for (j = 0; j < ms->num_wait_states; j++)
					{
						ms->wait_events[j] = strdup(wait_events[j]);
						ms->wait_targets[j] = strdup(wait_scripts[j]);
					}
				}

				if (theApp.m_PROF->ReadKeyValue(h, "loop", buffer, 80))
				{
					ms->loop = stricmp(buffer, "true") == 0;
				}
				if (theApp.m_PROF->ReadKeyValue(h, "transition", buffer, 80))
				{
					ms->transition_duration = atof(buffer);
				}
				if (theApp.m_PROF->ReadKeyValue(h, "reverse", buffer, 80))
				{
					ms->reverse = stricmp(buffer, "true") == 0;
				}
				if (theApp.m_PROF->ReadKeyValue(h, "interrupt", buffer, 80))
				{
					ms->interrupt = stricmp(buffer, "true") == 0;
				}
				else
				{
					ms->interrupt = true;
				}

				if (theApp.m_PROF->ReadKeyValue(h, "end", buffer, 80))
				{
					for (j = 0; j < num_sequences; j++)
					{
						if (strcmp(buffer, sequences[j].name) == 0)
						{
							ms->end = sequences + j;
							GDat->ConnectionCount[j]++;
							break;
						}
					}
				}
				theApp.m_PROF->CloseSection(h);
			}
		}

		//read in color values for states
		{
			char	buffer[80];

			h	=theApp.m_PROF->CreateSection("StateColors");

			if(h)
			{
				for(i=0;i < MT_NUM_MOTION_TYPES;i++)
				{
					if(theApp.m_PROF->ReadKeyValue(h, MotionTypeNames[i], buffer, 80))
					{
						GDat->StateColors[i]	=atoi(buffer);
					}
				}
				theApp.m_PROF->CloseSection(h);
			}
		}

		memset(seq_grid, 0, sizeof(MotionSequence *) * MT_NUM_MOTION_TYPES * MT_NUM_MOTION_TYPES);
		theApp.m_PROF->EnumerateSections(enum_callback, this);

		CalcUIData();

	}

	DAFILEDESC desc = "data\\characters\\";
	IFileSystem * data;
	if (theApp.m_DACOM->CreateInstance(&desc, (void **) &data) == GR_OK)
	{
		IFileSystem * anim_file;
		DAFILEDESC f_desc = anim_name;
		if (data->CreateInstance(&f_desc, (void **) &anim_file) == GR_OK)
		{
			anim_arch = theApp.m_ANIM->create_script_set_arch(anim_file);
			anim_file->Release();
		}

		data->Release();
	}
}


CharacterArchetype::~CharacterArchetype(void)
{
	if(mesh_name)
	{
		delete	[]	mesh_name;
	}
	if(anim_name)
	{
		delete	[]	anim_name;
	}

	if(script_names)
	{
		delete	[]	script_names;
		script_names	=NULL;
	}


	if(edges)
	{
		delete	[]	edges;
		edges	=NULL;
	}
	if(GDat)
	{
		if(GDat->BranchY)
		{
			delete	[]	GDat->BranchY;
		}
		if(GDat->NodeBoxes)
		{
			delete	[]	GDat->NodeBoxes;
		}
		if(GDat->ConnectionCount)
		{
			delete	[]	GDat->ConnectionCount;
		}
		if(GDat->ClusterNum)
		{
			delete	[]	GDat->ClusterNum;
		}
		if(GDat->BranchDepths)
		{
			delete	[]	GDat->BranchDepths;
		}
		if(GDat->ClusterBoxes)
		{
			delete	[]	GDat->ClusterBoxes;
		}
		delete	GDat;
	}

	num_edges = 0;

	theApp.m_ANIM->release_script_set_arch(anim_arch);

	num_sequences = 0;
	if(sequences)
	{
		delete	[]	sequences;
		sequences	=NULL;
	}

	if (arch_idx != INVALID_ARCHETYPE_INDEX) 
	{
		theApp.m_ENG->release_archetype(arch_idx);
	}

	num_attachments = 0;
	if(attachments)
	{
		delete	[]	attachments;
		attachments	=NULL;
	}

	num_transfers = 0;
	if(transfers)
	{
		delete	[]	transfers;
		transfers	=NULL;
	}
}

//

BaseObject* CharacterArchetype::create_instance(void)
{
	Character* c = new Character;

	if (c != NULL) 
	{
		c->deform = new DeformableObject;

		if (c->deform != NULL) 
		{
			// Create character hierarchy and deformable mesh.
			DAFILEDESC fdesc = "data\\characters";
			IFileSystem* file = NULL;
			theApp.m_DACOM->CreateInstance(&fdesc, (void **) &file);

			if (file) 
			{
				DeformPartMeshDesc m;
				m.mesh_parent = file;
				m.mesh_name = mesh_name;

				DeformPartDesc parts[1];
				parts[0].num_meshes = 1;
				parts[0].meshes = &m;
				parts[0].skeleton_parent = file;

				parts[0].anim_script_set = anim_arch;

				DeformDesc desc;
				desc.num_parts = 1;
				desc.parts = parts;

				c->deform->create(desc, c, NULL, c);

				c->index = c->deform->get_root();

				c->set_position(Vector(0, 0, 0));
				Matrix I; I.set_identity();
				c->deform->set_orientation(I);

				c->deform->set_up_axis(DeformableObject::POS_Y);
				c->deform->set_heading_axis(DeformableObject::POS_Z);
				
				file->Release();
			}

			c->current_sequence = NULL;
			c->archetype = this;

			char buffer[_MAX_PATH];
			sprintf(buffer, "%s_%d", m_name, cur_instance_num);
			c->name = new char[strlen(buffer) + 1];
			strcpy(c->name, buffer);
			cur_instance_num++;

			//get a list of scripts from the anim
			num_scripts		=c->deform->get_script_count();
			script_names	=new const char *[num_scripts];
			c->deform->get_scripts(script_names);

		// attachments.
			c->attachments = new Attachment[num_attachments];

			AttachmentArchetype * a = attachments;
			Attachment * ca = c->attachments;
			for (int i = 0; i < num_attachments; i++, a++, ca++)
			{
				ca->arch	= a;
				ca->active	= false;
				ca->parent	= INVALID_INSTANCE_INDEX;
				ca->child	= INVALID_INSTANCE_INDEX;
				if (a->active)
				{
					c->attach(i);
				}
			}


			c->command(CT_IDLE);
			theApp.m_ENG->update_instance(c->deform->get_root(), 1.0f);
			//c->Update(1.0f);

			add_ref();
		} 
		else 
		{
			if(c)
			{
				delete	c;
				c	=NULL;
			}
		}
	}

	if (c && edges == NULL)
	{
	// build edge list upon creation of first instance.
		num_edges = 0;

		CEdge temp_edge_list[4096];

	// ASSUMES 1 part for now.
		for (int i = 0; i < c->deform->num_parts; i++)
		{
			DeformablePart * part = c->deform->parts[i];
			DeformablePartArchetype * arch = part->meshes[0].arch;

			if (arch->new_format)
			{
				int face_idx = 0;
				FaceGroup * group = arch->face_groups;
				for (int g = 0; g < arch->face_group_cnt; g++, group++)
				{
				//
				// traverse faces.
				//
					int * fvc_idx = group->face_vertex_chain;
					for (int f = 0; f < group->face_cnt; f++, face_idx++)
					{
					// get indices into object vertex list.
    					int v0 = arch->vertex_batch_list[*(fvc_idx++)];
						int v1 = arch->vertex_batch_list[*(fvc_idx++)];
						int v2 = arch->vertex_batch_list[*(fvc_idx++)];

					// search for first edge.
						bool found = false;
						CEdge * edge = temp_edge_list;
						for (int e = 0; e < num_edges; e++, edge++)
						{
							if ((edge->v0 == v0 && edge->v1 == v1) ||
								(edge->v0 == v1 && edge->v1 == v0))
							{
								found = true;
								break;
							}
						}

						if (found)
						{
							edge->f1 = face_idx;
						}
						else
						{
							edge = temp_edge_list + num_edges++;
							edge->v0 = v0;
							edge->v1 = v1;
							edge->f0 = face_idx;
						}

					// 2nd edge.
						found = false;
						edge = temp_edge_list;
						for (e = 0; e < num_edges; e++, edge++)
						{
							if ((edge->v0 == v1 && edge->v1 == v2) ||
								(edge->v0 == v2 && edge->v1 == v1))
							{
								found = true;
								break;
							}
						}

						if (found)
						{
							edge->f1 = face_idx;
						}
						else
						{
							edge = temp_edge_list + num_edges++;
							edge->v0 = v1;
							edge->v1 = v2;
							edge->f0 = face_idx;
						}

					// 3rd edge.
						found = false;
						edge = temp_edge_list;
						for (e = 0; e < num_edges; e++, edge++)
						{
							if ((edge->v0 == v2 && edge->v1 == v0) ||
								(edge->v0 == v0 && edge->v1 == v2))
							{
								found = true;
								break;
							}
						}

						if (found)
						{
							edge->f1 = face_idx;
						}
						else
						{
							edge = temp_edge_list + num_edges++;
							edge->v0 = v2;
							edge->v1 = v0;
							edge->f0 = face_idx;
						}
					}
				}
			}
		}

		edges = new CEdge[num_edges];
		memcpy(edges, temp_edge_list, sizeof(CEdge) * num_edges);
	}

	return c;
}

void CharacterArchetype::delete_instance(BaseObject* obj)
{
	remove_ref();
}

//

bool CharacterArchetype::start_sequence(MotionType m, Character * c) const
{
	bool result = false;

	if (m >= 0 && m < MT_NUM_MOTION_TYPES)
	{
		MotionSequence * seq = seq_grid[c->motion][m];
		if (seq)
		{
		// If the new sequence requires waiting for an event, OR if the current sequence isn't
		// interruptable, set pending sequence instead of starting immediately.

			if (seq->num_wait_states || (c->current_sequence && !c->current_sequence->interrupt))
			{
				c->pending_sequence = seq;
				c->change_state = -1;
				result = true;
			}
			else
			{
				result = seq->start(c);
			}

			if (result)
			{
				if (c->motion == MT_DRAW_WEAPON)
				{
					c->weapon_ready = true;
					//DebugPrint("wr = true\n");
				}
				else if (c->motion == MT_HOLSTER_WEAPON)
				{
					c->weapon_ready = false;
					//DebugPrint("wr = false\n");
				}

				//DebugPrint("%s --> %s\n", MotionTypeNames[c->motion], MotionTypeNames[seq->type]);
				c->motion = seq->type;

				if ((c->motion == MT_IDLE_SPECIAL) && c->weapon_ready && (c->aim == INVALID_HANDLE_VALUE))
				{
					c->start_aim();
				}
			}
		}
		else
		{
		// NULL grid entries are okay, hell, even encouraged. 
		}
	}
	else
	{
		GENERAL_ERROR("Motion out of range in CharacterArchetype::start_sequence().\n");
	}

	return result;
}


//
// Character methods
//

Character::Character(void)
{
	deform					= NULL;
	motion					= MT_NONE;
	current_sequence		= NULL;
	pending_sequence		= NULL;
	weapon_ready			= false;
	special					= false;
	attachments				= NULL;
	num_pending_transfers	= 0;
	selected_gun			= NULL;
	aim						= INVALID_HANDLE_VALUE;
	head_aim				= INVALID_HANDLE_VALUE;

	head					=
	neck					= INVALID_INSTANCE_INDEX;
}

//

Character::~Character(void)
{
// index is just a reference to the root object, sold separately.
// Invalidate it now, so it doesn't get destroyed multiple times.
	index = INVALID_INSTANCE_INDEX;

	if(deform)
	{
		delete	deform;
		deform	=NULL;
	}
	if(attachments)
	{
		delete	[]	attachments;
	}
}

//

Vector Character::get_position(void)
{
	return theApp.m_ENG->get_position(deform->get_root());
}

//

void Character::set_position(Vector& new_pos)
{
	deform->set_position(new_pos);
}

//

Matrix Character::get_orientation(void)
{
	return theApp.m_ENG->get_orientation(deform->get_root());
}

//

void Character::set_orientation(Matrix& new_orient)
{
	theApp.m_ENG->set_orientation(deform->get_root(), new_orient);
}

//

bool Character::Update(float secs)
{
//	for (int i = 0; i < num_pending_transfers; i++)
//	{
//		pending_transfers[i]->execute(this);
//	}

	num_pending_transfers = 0;

	if (yaw != 0.0f)
	{
		Quaternion q(Vector(0, 1, 0), yaw * secs);
		Matrix Rq(q);

		deform->set_orientation(Rq * m_transform.get_orientation());
		yaw = 0.0f;
	}

	if (pending_sequence)
	{
		if (change_state != -1)
		{
			pending_sequence->start(this, change_state);
		}
	}
	else if (current_sequence)
	{
		current_sequence->update(this);
	}

//
// CHECKING FOR NO ACTIVE SCRIPTS isn't going to work for overlay motions. Need to keep track of
// multiple layers.
//
	int num_active_scripts = deform->get_num_active_scripts();
//DebugPrint("num active scripts: %d\n", num_active_scripts);
	if ((num_active_scripts == 0) || ((num_active_scripts > 0) && (aim != INVALID_HANDLE_VALUE)))
	{
		if (pending_sequence)
		{
			pending_sequence->start(this, 0);
		}
		else if (current_sequence && current_sequence->end)
		{
			MotionSequence * end = current_sequence->end;
			if (end->start(this, 0))
			{
				if (motion == MT_DRAW_WEAPON)
				{
					weapon_ready = true;
					//DebugPrint("wr = true\n");
				}
				else if (motion == MT_HOLSTER_WEAPON)
				{
					weapon_ready = false;
					//DebugPrint("wr = false\n");
				}


				//DebugPrint("%s --> %s\n", MotionTypeNames[motion], MotionTypeNames[end->type]);
				motion = end->type;

				if ((motion == MT_IDLE_SPECIAL) && weapon_ready && (aim == INVALID_HANDLE_VALUE))
				{
					start_aim();
				}
			}
		}
	}

	const Vector & pos = m_transform.get_position();

	velocity = (pos - last_pos) / secs;
	last_pos = pos;

// TODO: check height differential and/or normal of surface under character
// in order to stop movement over impossible obstacles.
	Vector char_pos = theApp.m_ENG->get_position(deform->get_root());
//	float height = The_level.get_height(char_pos);
	float height = 0.0f;
	deform->set_floor_height(height);
	last_height = height;

// deal with IK/aiming.
	if (aim != INVALID_HANDLE_VALUE)
	{
//		int mx = theApp.m_input->get_mouse_x();
//		int my = theApp.m_input->get_mouse_y();
		int mx = 69;
		int my = 69;

		int dx = mx - xIK;
		int dy = my - yIK;

	// 10 pixels to 1 degree. 
		float y = -Degrees2Radians(dx * 0.25f);
		float p = Degrees2Radians(dy * 0.25f);

	// LIMIT MOTION - should be archetype-dependent.
		y = __max(Degrees2Radians(-3.5), __min(y, Degrees2Radians(3.5)));
		p = __max(Degrees2Radians(-3.5), __min(p, Degrees2Radians(3.5)));

		Matrix Rrel = Matrix::rotate_y(y) * Matrix::rotate_z(p);
		R_IK *= Rrel;

		Vector fp, fd;
//		selected_gun->get_fire_vector(fp, fd);

		Vector look_target = fp + fd * 100;
		Vector dl = look_target - theApp.m_ENG->get_position(head);
		dl.normalize();

		const Vector & k = dl;
		Vector i(0, -1, 0);
		Vector j = cross_product(k, i);
		j.normalize();
		i = cross_product(j, k);
		i.normalize();
		Rhead.set_i(i);
		Rhead.set_j(j);
		Rhead.set_k(k);
	}

	return true;
}

//

#define CHECK(n) assert((n) == GR_OK)

//

//#define CHARACTER_SHADOWS

//

void Character::Render(ICamera* the_camera, U32 flags)
{
//	if (!render_chars) return;

// always render once
	static bool rendered_once = false;

// Do check against viewport:
//	RECT rect;
// 	if (!rendered_once || deform->visible_rect(rect, the_camera))
	{
		int w, h;
		theApp.GetPerspectiveViewport(w, h);
//		if (!rendered_once || 
//			((rect.top >= 0 && rect.top < h && rect.bottom >= 0 && rect.bottom < h) &&
//			(rect.left >= 0 && rect.left < w && rect.right >= 0 && rect.right < w)))
		{
//			theApp.SetDefaultTextureBlend();
//			theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
			int lod = 0;
			deform->deform(&lod);
			deform->render(the_camera, &lod);

			rendered_once = true;
		}
	}

#ifdef CHARACTER_SHADOWS

	theApp.m_BATCH->set_modelview(the_camera->get_inverse_transform());

	if (0)
	{
	//
	// Locate silhouette edges.
	// 1. classify all faces as facing light or not.
	// 2. traverse edges, find those that are between opposite-facing faces, or facing with no adjacent face.

#define MAX_FACES 2048
		bool	facing_light[MAX_FACES];
		Vector	face_normal[MAX_FACES];

	//
	// 1. GET GROUP OF TERRAIN TRIANGLES THAT COULD POSSIBLY CONTAIN SHADOW. Use bounding box or whatever.
	// 2. Get group of unique planes in triangles.
	// 3. Project all vertices onto each plane.
	// 4. 
	//

		Link<GameLight *> * node = The_level.m_light_list.get_head();
		if (node)
		{
			Vector L = node->obj->get_position();
			Vector dL;
			if (node->obj->IsInfinite())
			{
				node->obj->GetDirection(dL);
			}
			else
			{
				dL = pos - L;
			}

			Vector vert[3];

			int face_idx = 0;
			for (int i = 0; i < deform->num_parts; i++)
			{
				DeformablePart * part = deform->parts[i];
				DeformablePartArchetype * arch = part->meshes[0].arch;

				if (arch->new_format)
				{
					FaceGroup * group = arch->face_groups;
					for (int g = 0; g < arch->face_group_cnt; g++, group++)
					{
					//
					// traverse faces.
					//
						int * fvc_idx = group->face_vertex_chain;
						for (int f = 0; f < group->face_cnt; f++, face_idx++)
						{
							Vector * v0 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx++)];
							Vector * v1 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx++)];
							Vector * v2 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx++)];

							Vector dv0 = *v1 - *v0;
							Vector dv1 = *v2 - *v0;
							face_normal[face_idx] = cross_product(dv0, dv1);

							float dot = dot_product(dL, face_normal[face_idx]);
							if (dot < 0)
							{
								facing_light[face_idx] = true;
							}
							else
							{
								facing_light[face_idx] = false;
							}
						}
					}
				}
			}

		// traverse edges.
			CEdge * sedges[4096];
			int num_sedges = 0;

			CEdge * edge = get_arch()->edges;
			for (i = 0; i < get_arch()->num_edges; i++, edge++)
			{
				if (facing_light[edge->f0])
				{
				// f0 facing light, and f1 not (or no f1 present).
					if (edge->f1 == -1)
					{
						sedges[num_sedges] = edge;
						sedges[num_sedges]->N = face_normal[edge->f0];
						num_sedges++;
					}
					else if (!facing_light[edge->f1])
					{
						sedges[num_sedges] = edge;
						sedges[num_sedges]->N = 0.5f * (face_normal[edge->f0] + face_normal[edge->f1]);
						num_sedges++;
					}
				}
				else if ((edge->f1 != -1) && facing_light[edge->f1])
				{
				// f1 facing light, f0 not.
					sedges[num_sedges] = edge;
					sedges[num_sedges]->N = 0.5f * (face_normal[edge->f0] + face_normal[edge->f1]);

					num_sedges++;
				}
			}

		// WE HAVE SILHOUETTE EDGES, build shadow volume.

			//int num_shadow_volume_faces = (num_sedges << 1) - 2;

			DeformablePart * part = deform->parts[0];
			DeformablePartArchetype * arch = part->meshes[0].arch;

		// compute shadow volume verts. extend below terrain.
			Vector sv_verts[MAX_FACES];
			Vector sv_edges[MAX_FACES];
			Vector * dst = sv_verts;
			Vector * src = part->transformed_vertices;
			Vector * sve = sv_edges;
			for (i = 0; i < arch->object_vertex_cnt; i++, dst++, src++, sve++)
			{
				*sve = *src - L;
				*dst = L + 1.1f * (*sve);
			}

			struct SVFace
			{
				float	cdot;
			};

			Vector view = -the_camera->get_transform().get_k();

			SVFace sv_faces[MAX_FACES];
			SVFace * face = sv_faces;
			CEdge ** s_ptr = sedges;
			for (i = 0; i < num_sedges; i++, face++, s_ptr++)
			{
				CEdge * edge = *s_ptr;
				Vector * e0 = sv_edges + edge->v0;
				Vector * e1 = sv_edges + edge->v1;
				
				Vector N = cross_product(*e0, *e1);
				float dot = dot_product(N, edge->N);
				if (dot < 0)
				{
					N = -N;
					N.normalize();
				}

			// now compute camera dot...
				face->cdot = dot_product(N, view);
			}

	// NEED FACING INFORMATION FOR SHADOW POLYS. FACE OUT FROM SHADOW (cw).

		// DISABLE DRAWING TO COLOR BUFFER.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_PLANEMASK,			0));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,	TRUE));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_SRCBLEND,			D3DBLEND_ZERO));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_DESTBLEND,			D3DBLEND_ONE));

		// DISABLE DRAWING TO ZBUFFER.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_ZWRITEENABLE,		FALSE));
		// ENABLE DRAWING TO STENCIL BUFFER.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILENABLE,		TRUE));

		// Set stencil to always pass, write 1 to stencil buffer if passes z-test.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILFUNC,		D3DCMP_ALWAYS));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILREF,			1));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILMASK,		0xffffffff));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILWRITEMASK,	0xffffffff));

			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILZFAIL,		D3DSTENCILOP_KEEP));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILFAIL,		D3DSTENCILOP_KEEP));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILPASS,		D3DSTENCILOP_REPLACE));

#if 0
			theApp.SetNoTexture();
			theApp.m_BATCH->set_render_state(D3DRS_PLANEMASK,		0xffffffff);
			theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,	FALSE);
			theApp.m_BATCH->set_render_state(D3DRS_STENCILENABLE,	FALSE);
			theApp.m_BATCH->set_render_state(D3DRS_ZWRITEENABLE,		TRUE);
			PrimitiveBuilder pb(theApp.m_BATCH);
			pb.Begin(GL_TRIANGLES);
			pb.Color3ub(255, 0, 0);

				face = sv_faces;
				s_ptr = sedges;
				for (i = 0; i < num_sedges; i++, face++, s_ptr++)
				{
					if (face->cdot < 0)
					{
						pb.Color3ub(rand() & 0xff, 255, rand() & 0xff);
						CEdge * s = *s_ptr;
						Vector * v0 = sv_verts + s->v0;
						Vector * v1 = sv_verts + s->v1;

						pb.Vertex3f(L.x, L.y, L.z);
						pb.Vertex3f(v0->x, v0->y, v0->z);
						pb.Vertex3f(v1->x, v1->y, v1->z);
					}
				}

			pb.End();
#else
		// draw front-side of shadow volume.
			PrimitiveBuilder pb(theApp.m_BATCH);
			pb.Begin(PB_TRIANGLES);

				face = sv_faces;
				s_ptr = sedges;
				for (i = 0; i < num_sedges; i++, face++, s_ptr++)
				{
					if (face->cdot < 0)
					{
						pb.Vertex3f(L.x, L.y, L.z);

						CEdge * s = *s_ptr;
						Vector * v0 = sv_verts + s->v0;
						Vector * v1 = sv_verts + s->v1;

						pb.Vertex3f(v0->x, v0->y, v0->z);
						pb.Vertex3f(v1->x, v1->y, v1->z);
					}
				}

			pb.End();
#endif

		// now draw back side, writing zeros into stencil buffer.
			theApp.m_BATCH->set_render_state(D3DRS_STENCILREF,		0);

			pb.Begin(PB_TRIANGLES);

				face = sv_faces;
				s_ptr = sedges;
				for (i = 0; i < num_sedges; i++, face++, s_ptr++)
				{
					if (face->cdot >= 0)
					{
						pb.Vertex3f(L.x, L.y, L.z);

						CEdge * s = *s_ptr;
						Vector * v0 = sv_verts + s->v0;
						Vector * v1 = sv_verts + s->v1;

						pb.Vertex3f(v0->x, v0->y, v0->z);
						pb.Vertex3f(v1->x, v1->y, v1->z);
					}
				}

			pb.End();

		// RESTORE STATES.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_PLANEMASK,			0xffffffff));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_ZWRITEENABLE,		TRUE));

		// NOW SHADOWED PIXELS HAVE STENCIL VALUE 1.
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_ZENABLE,			FALSE));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,	TRUE));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA));

			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILREF,			1));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILFUNC,		D3DCMP_EQUAL));
			CHECK(theApp.m_BATCH->set_render_state(D3DRS_STENCILPASS,		D3DSTENCILOP_KEEP));

		// FILL the screen with a big gray poly, writing only where the stencil value is 1.
			Matrix Id; Id.set_identity();
			theApp.m_BATCH->set_modelview(Id);

			float w = the_camera->get_znear() * tan(Degrees2Radians(the_camera->get_fovx()));
			float h = the_camera->get_znear() * tan(Degrees2Radians(the_camera->get_fovy()));

			theApp.SetNoTexture();
			pb.Begin(PB_TRIANGLES);

				pb.Color4ub(16, 16, 16, 128);

				pb.Vertex3f(-w,  h, -1);
				pb.Vertex3f( w,  h, -1);
				pb.Vertex3f(-w, -h, -1);

				pb.Vertex3f( w,  h, -1);
				pb.Vertex3f( w, -h, -1);
				pb.Vertex3f(-w, -h, -1);

			pb.End();

			theApp.m_BATCH->set_render_state(D3DRS_STENCILENABLE,	FALSE);
			theApp.m_BATCH->set_render_state(D3DRS_ZENABLE,			TRUE);

#if 0
		// DRAW SILHOUETTE EDGES (DEBUG)
			theApp.SetNoTexture();
			DWORD zfunc;
			theApp.m_BATCH->get_render_state(D3DRS_ZFUNC, &zfunc);
			theApp.m_BATCH->set_render_state(D3DRS_ZFUNC, D3DCMP_ALWAYS);

			DeformablePart * part = deform->parts[0];
			DeformablePartArchetype * arch = part->meshes[0].arch;

			PrimitiveBuilder pb(theApp.m_BATCH);
			pb.Begin(GL_LINES);
				pb.Color3ub(255, 0, 0);

				CEdge ** s_ptr = sedges;
				for (i = 0; i < num_sedges; i++, s_ptr++)
				{
					CEdge * s = *s_ptr;
					Vector * v0 = part->transformed_vertices + s->v0;
					Vector * v1 = part->transformed_vertices + s->v1;

					pb.Vertex3f(v0->x, v0->y, v0->z);
					pb.Vertex3f(v1->x, v1->y, v1->z);
				}


			pb.End();

			theApp.m_BATCH->set_render_state(D3DRS_ZFUNC, zfunc);
#endif

		}
	}
	else if (1)
	{
	//
	// Fakeoid shadow. Compute face normals (no normalization, though). Expensive,
	// but better than drawing every poly. Would be preferable to have Deform somehow
	// intelligently maintain correct normals? Expensive any way you slice it.
	//

		float ty = The_level.GetTerrainHeight(pos.x, pos.z);
		float dty = ty + 0.1;

		theApp.SetNoTexture();
		theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,	TRUE);
		theApp.m_BATCH->set_render_state(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA);
		theApp.m_BATCH->set_render_state(D3DRS_DESTBLEND,		D3DBLEND_INVSRCALPHA);

		int shadow_polys = 0;

		PrimitiveBuilder pb(theApp.m_BATCH);
		pb.Color4ub(0, 0, 0, 128);

		Link<GameLight *> * node = The_level.m_light_list.get_head();
		if (node)
		{
			Vector L = node->obj->get_position();

#if 1
			pb.Begin(PB_TRIANGLES);

			Vector dL = pos - L;

			float num = L.y - dty;

			Vector vert[3];

			for (int i = 0; i < deform->num_parts; i++)
			{
				DeformablePart * part = deform->parts[i];
				DeformablePartArchetype * arch = part->meshes[0].arch;

		// TODO: pre-project all vertices, look up results. Lots of sharing in characters.
		// But also lots of faces & verts that don't participate in shadow.

				if (arch->new_format)
				{
					FaceGroup * group = arch->face_groups;
					for (int g = 0; g < arch->face_group_cnt; g++, group++)
					{
					//
					// traverse faces.
					//
						int * fvc_idx = group->face_vertex_chain;
						for (int f = 0; f < group->face_cnt; f++)
						{
							Vector * v0 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx+0)];
							Vector * v1 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx+1)];
							Vector * v2 = part->transformed_vertices + arch->vertex_batch_list[*(fvc_idx+2)];

							if (v0->y < L.y && v1->y < L.y && v2->y < L.y)
							{
								Vector dv0 = *v1 - *v0;
								Vector dv1 = *v2 - *v0;
								Vector N = cross_product(dv0, dv1);
								//if (N.y >= 0)
								if (dot_product(dL, N) < 0)
								{
									float d = num / (v0->y - L.y);
									float x = (1.0 + d) * L.x - d * v0->x;
									float z = (1.0 + d) * L.z - d * v0->z;
									pb.Vertex3f(x, dty, z);

									d = num / (v1->y - L.y);
									x = (1.0 + d) * L.x - d * v1->x;
									z = (1.0 + d) * L.z - d * v1->z;
									pb.Vertex3f(x, dty, z);

									d = num / (v2->y - L.y);
									x = (1.0 + d) * L.x - d * v2->x;
									z = (1.0 + d) * L.z - d * v2->z;
									pb.Vertex3f(x, dty, z);

									shadow_polys++;
								}
							}

							fvc_idx += 3;
						}
					}
				}
				else
				{
					for (int f = 0; f < arch->face_cnt; f++)
					{
						S32 first = arch->face_vertices[f];
						int * chain = arch->object_vertex_chain + first;

						Vector * v0 = part->transformed_vertices + *chain++;
						Vector * v1 = part->transformed_vertices + *chain++;
						Vector * v2 = part->transformed_vertices + *chain++;

						if (v0->y < L.y && v1->y < L.y && v2->y < L.y)
						{
							Vector dv0 = *v1 - *v0;
							Vector dv1 = *v2 - *v0;
							Vector N = cross_product(dv0, dv1);
							//if (N.y >= 0)
							if (dot_product(dL, N) < 0)
							{
								float inv_denom = 1.0/(v0->y - L.y);
								float x = (L.x * v0->y - v0->x * L.y + ty * (v0->x - L.x)) * inv_denom;
								float z = (L.z * v0->y - v0->z * L.y + ty * (v0->z - L.z)) * inv_denom;
								pb.Vertex3f(x, dty, z);

								inv_denom = 1.0/(v1->y - L.y);
								x = (L.x * v1->y - v1->x * L.y + ty * (v1->x - L.x)) * inv_denom;
								z = (L.z * v1->y - v1->z * L.y + ty * (v1->z - L.z)) * inv_denom;
								pb.Vertex3f(x, dty, z);

								inv_denom = 1.0/(v2->y - L.y);
								x = (L.x * v2->y - v2->x * L.y + ty * (v2->x - L.x)) * inv_denom;
								z = (L.z * v2->y - v2->z * L.y + ty * (v2->z - L.z)) * inv_denom;
								pb.Vertex3f(x, dty, z);
								shadow_polys++;
							}
						}
					}
				}
			}

			pb.End();
#else

			pb.Begin(GL_LINES);

			for (int i = 0; i < deform->num_parts; i++)
			{
				DeformablePart * part = deform->parts[i];
				DeformablePartArchetype * arch = part->meshes[0].arch;

				if (arch->new_format)
				{
					const Vector * vp = part->transformed_vertices;
					for (int j = 0; j < arch->object_vertex_cnt; j++, vp++)
					{
						Vector ray = *vp - L;
						ray.normalize();

						Vector p, N;

						if (The_level.the_terrain->collide_ray(p, N, L, ray))
						{
							pb.Vertex3f(p.x, p.y, p.z);
							pb.Vertex3f(p.x, p.y+1, p.z);
						}
					}
				}
			}

			pb.End();
#endif
		}

		theApp.m_BATCH->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
	}
#endif
}

//

void Character::command(CommandType com, U32 param)
{
//DebugPrint("char cmd: %s\n", CommandTypeNames[com]);
	MotionType m = MT_NONE;

	switch (com)
	{
		case CT_IDLE:
			m = (special) ? MT_IDLE_SPECIAL : MT_IDLE;
			break;

		case CT_STOP:
			m = MT_STOP;
			break;

		case CT_WALK:
			m = (special) ? MT_WALK_SPECIAL : MT_WALK;
			break;

		case CT_RUN:
			m = MT_RUN;
			break;

		case CT_TURN_LEFT:
			switch (motion)
			{
				case MT_WALK:
				case MT_WALK_SPECIAL:
				case MT_RUN:
					yaw = get_arch()->get_turn_rate();
					break;

				default:
					yaw = 0;
					m = (special) ? MT_TURN_LEFT_SPECIAL : MT_TURN_LEFT;
					break;
			}
			break;

		case CT_TURN_RIGHT:
			switch (motion)
			{
				case MT_WALK:
				case MT_WALK_SPECIAL:
				case MT_RUN:
					yaw = -get_arch()->get_turn_rate();
					break;

				default:
					yaw = 0;
					m = (special) ? MT_TURN_RIGHT_SPECIAL : MT_TURN_RIGHT;
					break;
			}
			break;

		case CT_MOVE_LEFT:
			m = MT_MOVE_LEFT;
			break;

		case CT_MOVE_RIGHT:
			m = MT_MOVE_RIGHT;
			break;

		case CT_TOGGLE_SPECIAL:
			special = !special;
			break;

		case CT_TOGGLE_WEAPON_READY:
			if (weapon_ready)
			{
				m = MT_HOLSTER_WEAPON;
			}
			else
			{
				m = MT_DRAW_WEAPON;
			}
			break;

		case CT_FIRE_WEAPON:
			if (selected_gun)
			{
				if (weapon_ready)
				{
//					if (selected_gun->fire())
					{
						m = MT_FIRE_WEAPON;
					}
				}
				else
				{
					m = MT_DRAW_WEAPON;
				}
			}
			else
			{
				m = MT_FIRE_WEAPON;
			}
			break;

		case CT_ATTACK:
		{
			int mt = MT_ATTACK0 + param;
			m = (MotionType) mt;
			break;
		}
	}

	if (m != MT_NONE)
	{
		if (m == MT_HOLSTER_WEAPON)
		{
			end_aim();
		}

	 	get_arch()->start_sequence(m, this);
	}
}

//

void COMAPI Character::on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator)
{
	int count = event_iterator.get_event_count();
	for (int i = 0; i < count; i++)
	{
		float t				= event_iterator.get_event_time(i);
		unsigned int type	= event_iterator.get_event_type(i);
		void * data			= event_iterator.get_event_data(i);

		switch (type)
		{
			case NAMED_EVENT:
			{
				char * str = (char *) data;
//				DebugPrint("channel event: %s\n", str);

				if (pending_sequence)
				{
					for (int j = 0; j < pending_sequence->num_wait_states; j++)
					{
						if (strcmp(pending_sequence->wait_events[j], str) == 0)
						{
							change_state = j;
							break;
						}
					}
				}

				if (get_arch()->num_transfers)
				{
					const CharacterArchetype * arch = get_arch();
					Transfer * t = arch->transfers;
					for (int j = 0; j < arch->num_transfers; j++, t++)
					{
						if (strcmp(t->trigger, str) == 0)
						{
							if (attachments[t->src].active)
							{
								pending_transfers[num_pending_transfers++] = t;
							}
						}
					}
				}

				break;
			}
			case CHANNEL_BEGIN:
				break;
			case CHANNEL_END:
			//
			// NEED TO VERIFY THAT THIS END EVENT IS FROM THE RIGHT MOTION. In cases of
			// multiple layers of transitions, this may be an end event from a previous
			// motion, not the current "main" motion. 
			//
			// INSTEAD OF DOING IT HERE, just detect when no motions are playing in Update().
			default:
				break;
		}
	}
}

//

void Character::attach(int idx, INSTANCE_INDEX child)
{
	const CharacterArchetype * arch = get_arch();

	if (idx < arch->num_attachments)
	{
		const AttachmentArchetype * a = arch->attachments + idx;
		Attachment * ca = attachments + idx;

		if (ca->active)
		{
			GENERAL_ERROR("Character::attach() - child object already attached.\n");
		}
		else
		{
  		// HACK - hard-coded type. No good.
			if (child == INVALID_INSTANCE_INDEX)
			{
//				ObjectType type = The_level.LookupType(a->child_object);

			// ASSUME CHILD OBJECTS ARE PHYSICAL OBJECTS?

//	  			PhysicalObject * obj = (PhysicalObject *) The_level.CreateObject(type, a->child_object);
//				child = obj->get_index();

			// for now, don't collide while connected to character.
//				obj->set_tangible(false);
//				obj->sleep();
			}

			HardpointDesc * hpd = deform->hardpoints;
			for (unsigned int h = 0; h < deform->num_hardpoints; h++, hpd++)
			{
				if (strcmp(hpd->name, a->parent_hp) == 0)
				{
					if (theApp.m_HARDPOINT->connect(hpd->object, a->parent_hp, child, a->child_hp) == 0)
					{
						ca->parent	= hpd->object;
						ca->child	= child;

						ca->active = true;

						theApp.m_MODEL->update_tree(deform->get_root());

						if (a->select)
						{
							selected_gun = (Gun *) theApp.m_ENG->get_user_data(child);
						}
					}
					else
					{
						char temp[80];
						sprintf(temp, "Unable to attach object %s to character %s.\n", a->child_object, arch->m_name);
						GENERAL_ERROR(temp);
					}
					break;
				}
			}
		}
	}
}

//

INSTANCE_INDEX find_child(INSTANCE_INDEX root, const char * name)
{
	INSTANCE_INDEX result = INVALID_INSTANCE_INDEX;

	if (theApp.m_MODEL->is_named(root, name))
	{
		result = root;
	}
	else
	{
		INSTANCE_INDEX child = theApp.m_MODEL->get_child(root);
		while ((child != INVALID_INSTANCE_INDEX) && (result == INVALID_INSTANCE_INDEX))
		{
			result = find_child(child, name);
			child = theApp.m_MODEL->get_child(root, child);
		}
	}

	return result;
}

//

void Character::start_aim(void)
{
	assert(aim == INVALID_HANDLE_VALUE);

	char parent_name[] = "Bip01 R Clavicle"; 
	char child_name[] = "Bip01 R Hand";  

	INSTANCE_INDEX parent	= find_child(deform->get_root(), parent_name);
	INSTANCE_INDEX child	= find_child(deform->get_root(), child_name);

	R_IK = theApp.m_ENG->get_orientation(child);

	AimDesc desc("aim", parent, child, &p_IK, &R_IK);
	desc.flags = AimDesc::AD_EE_ORIENT;
	desc.set_damping(3);

	aim = deform->start_aim(desc, 0.1);

//	xIK = theApp.m_input->get_mouse_x();
//	yIK = theApp.m_input->get_mouse_y();
	xIK = 69;
	yIK = 69;

// do head aim.
	if (head == INVALID_INSTANCE_INDEX)
	{
		head = find_child(deform->get_root(), "Bip01 Head");
		neck = find_child(deform->get_root(), "Bip01 Spine3");
	}

	Rhead = theApp.m_ENG->get_orientation(child);

	AimDesc desc2("head aim", neck, head, &p_IK, &Rhead);
	desc2.flags = AimDesc::AD_EE_ORIENT;// | AimDesc::AD_IGNORE_LIMITS;
	desc2.set_damping(3);

	head_aim = deform->start_aim(desc2, 0.1);
}

//

void Character::end_aim(void)
{
	if (aim != INVALID_HANDLE_VALUE)
	{
		deform->end_aim(aim);
		aim = INVALID_HANDLE_VALUE;

		deform->end_aim(head_aim);
		head_aim = INVALID_HANDLE_VALUE;
	}
}

//

void Character::resupply(void)
{
	Attachment * a = attachments;
	for (int i = 0; i < get_arch()->num_attachments; i++, a++)
	{
		if (a->arch->active && (a->child == INVALID_INSTANCE_INDEX))
		{
			attach(i);
		}
	}
}




//get rects for clusters, nodes, and states
void	CharacterArchetype::CalcClusterExtents(int idx)
{
	int		i, x, y, mcnt, max, cury, miny, maxy, XCnt;
	int		MaxYSize, XSize, MaxXSize, MaxXCnt, j, XIndexes[64];
	float	SpacingScale;

	assert(idx >= 0);

	//get the max branch depths
	for(i=mcnt=0,max=-1;i < num_sequences;i++)
	{
		if(GDat->ClusterNum[i] != idx)
		{
			continue;
		}
		if(max < GDat->BranchDepths[i])
		{
			max		=GDat->BranchDepths[i];
			mcnt	=0;
		}
		else if(max == GDat->BranchDepths[i])
		{
			mcnt++;
		}
	}

	//need to clear out BranchY
	memset(GDat->BranchY, 0, sizeof(int) * num_sequences);

	//make another pass and assign y for first max depth branch
	for(i=0;i < num_sequences;i++)
	{
		if(GDat->ClusterNum[i] != idx)
		{
			continue;
		}

		if(max == GDat->BranchDepths[i])
		{
			MotionSequence	*ms;

			//reset visited
			for(x=0;x < num_sequences;x++)
			{
				sequences[x].bVisited	=FALSE;
			}

			for(cury=0,ms=&sequences[i];ms;ms=ms->end,cury++)
			{
				if(ms->bVisited)
				{
					break;
				}
				ms->bVisited	=TRUE;

				GDat->BranchY[(((int)ms - (int)sequences) / (int)sizeof(MotionSequence))]	=cury;
			}

			//build y's for the other nodes int the cluster based on the max depth branch
			//recurse to the nodes pointing to the main branch nodes... make sense?
			//reset visited
			for(x=0;x < num_sequences;x++)
			{
				sequences[x].bVisited	=FALSE;
			}
			for(cury=0,ms=&sequences[i];ms;ms=ms->end,cury++)
			{
				if(ms->bVisited)
				{
					break;
				}
				ms->bVisited	=TRUE;

				SetNodesY(ms, cury);
			}

			break;
		}
	}

	//find the max extent of Y (some may go back into negative due to hierarchy)
	miny	=696969;
	maxy	=-696969;
	for(i=0;i < num_sequences;i++)
	{
		if(GDat->ClusterNum[i] != idx)
		{
			continue;
		}

		if(miny > GDat->BranchY[i])
		{
			miny	=GDat->BranchY[i];
		}
		if(maxy < GDat->BranchY[i])
		{
			maxy	=GDat->BranchY[i];
		}
	}

	//find max x extent for whole thing (this is all slow looking)
	MaxXCnt	=-696969;

	for(j=miny, XCnt=0;j <= maxy;j++)
	{
		for(i=0;i < num_sequences;i++)
		{
			if(GDat->ClusterNum[i] != idx)
			{
				continue;
			}
			if(GDat->BranchY[i] == j)
			{
				XCnt++;
			}
		}
		if(XCnt > MaxXCnt)
		{
			MaxXCnt	=XCnt;
		}
	}

	//grab extents based on row counts and text sizes
	for(j=miny, XCnt=0, MaxXSize=-6969;j <= maxy;j++, XCnt=0)	//reset xcnt each loop
	{
		for(i=0;i < num_sequences;i++)
		{
			if(GDat->ClusterNum[i] != idx)
			{
				continue;
			}
			if(GDat->BranchY[i] == j)
			{
				XIndexes[XCnt++]	=i;
			}
		}

		//check text sizes for this Y
		for(i=XSize=0;i < XCnt;i++)
		{
			XSize	+=GDat->NodeBoxes[XIndexes[i]].TextSize.cx;
		}

		//add space for text borders and in betweens
		XSize	+=XCnt * 32 + 16;

		if(MaxXSize < XSize)
		{
			MaxXSize	=XSize;
		}
	}

	//use extents to center non maxed rows
	for(j=miny, XCnt=0;j <= maxy;j++, XCnt=0)	//reset xcnt each loop
	{
		for(i=0;i < num_sequences;i++)
		{
			if(GDat->ClusterNum[i] != idx)
			{
				continue;
			}
			if(GDat->BranchY[i] == j)
			{
				XIndexes[XCnt++]	=i;
			}
		}

		//check text sizes for this Y
		for(i=XSize=0;i < XCnt;i++)
		{
			XSize	+=GDat->NodeBoxes[XIndexes[i]].TextSize.cx;
		}

		//add space for text borders and in betweens
		XSize	+=XCnt * 32 + 16;

		if(MaxXSize != XSize)
		{
			SpacingScale	=(float)MaxXSize / (float)XSize;
		}
		else
		{
			SpacingScale	=1.0f;
		}

		//find cluster relative node rects
		y	=16 + (j - miny) * GDat->NodeBoxes[0].TextSize.cy + (j - miny) * 32;
		x	=(int)(16.0f * SpacingScale);

		for(i=0;i < XCnt;i++)
		{
			GDat->NodeBoxes[XIndexes[i]].NodeRect.top		=y;
			GDat->NodeBoxes[XIndexes[i]].NodeRect.bottom	=y + GDat->NodeBoxes[XIndexes[i]].TextSize.cy + 16;
			GDat->NodeBoxes[XIndexes[i]].NodeRect.left		=x;
			GDat->NodeBoxes[XIndexes[i]].NodeRect.right		=x + GDat->NodeBoxes[XIndexes[i]].TextSize.cx + 16;

			x	+=(GDat->NodeBoxes[XIndexes[i]].TextSize.cx + (int)(32.0f * SpacingScale));

			GDat->NodeBoxes[XIndexes[i]].Color	=GDat->StateColors[sequences[XIndexes[i]].type];
		}
	}

	//hack... for now all y sizes the same (single line o text)
	MaxYSize	=(maxy - miny + 1) * GDat->NodeBoxes[0].TextSize.cy;

	//add space for text borders and in betweens
	MaxYSize	+=(maxy - miny + 1) * 32 + 16;

	GDat->ClusterBoxes[idx].NodeRect.top	=0;
	GDat->ClusterBoxes[idx].NodeRect.left	=0;
	GDat->ClusterBoxes[idx].NodeRect.bottom	=MaxYSize;
	GDat->ClusterBoxes[idx].NodeRect.right	=MaxXSize;
	GDat->ClusterBoxes[idx].TextSize.cx		=MaxXSize;
	GDat->ClusterBoxes[idx].TextSize.cy		=MaxYSize;
	GDat->ClusterBoxes[idx].Color			=RGB(169,69,69);	
}

void	CharacterArchetype::CalcStateExtents(void)
{
	int	i, j;

	for(i=0;i < num_sequences;i++)
	{
		for(j=0;j < MT_NUM_MOTION_TYPES;j++)
		{
			CRect	ofsrect(GDat->NodeBoxes[i].NodeRect);

			ofsrect	+=	GDat->ClusterBoxes[GDat->ClusterNum[i]].NodeRect.TopLeft();

			if(sequences[i].type == j)
			{
				GDat->StateBoxes[j].NodeRect.UnionRect(&GDat->StateBoxes[j].NodeRect, &ofsrect);
			}
		}
	}
}

void	CharacterArchetype::SetClusterColor(int cidx, COLORREF crgb)
{
	int	i;

	for(i=0;i < num_sequences;i++)
	{
		if(GDat->ClusterNum[i] == cidx)
		{
			GDat->NodeBoxes[i].Color	=crgb;
		}
	}
}

//set all nodes of state to crgb
void	CharacterArchetype::SetStateColor(int state, COLORREF crgb)
{
	int	i;

	for(i=0;i < num_sequences;i++)
	{
		if(sequences[i].type == state)
		{
			GDat->NodeBoxes[i].Color	=crgb;
		}
	}
}

void	CharacterArchetype::SetNodesY(MotionSequence *ms, int y)
{
	int	i;

	assert(ms);

	for(i=0;i < num_sequences;i++)
	{
		if(sequences[i].end == ms)
		{
			if(!GDat->BranchY[i])
			{
				SetNodesY(&sequences[i], y - 1);
				GDat->BranchY[i]	=y - 1;
			}
		}
/*		if(ms->end == &sequences[i])
		{
			if(!BranchY[i + (((int)ms - (int)&sequences[i]) / (int)sizeof(MotionSequence))])
			{
				SetNodesY(&sequences[i], y - 1);
			}
		}
*/	}

}

void	CharacterArchetype::FindAttached(int i)
{
	int	j;

	for(j=0;j < num_sequences;j++)
	{
		if(i==j)
		{
			continue;
		}

		if(sequences[j].end == &sequences[i])
		{
			if(GDat->ClusterNum[j] == -1)
			{
				GDat->ClusterNum[j]	=GDat->CurCluster;
				FindAttached(j);
			}
		}
	}
}

void	CharacterArchetype::GetWorldSpaceNodeRect(CRect *ofsrect, int i)
{
	assert(ofsrect);
	assert(i >= 0);

	*ofsrect	=GDat->NodeBoxes[i].NodeRect;

	*ofsrect	+=GDat->ClusterBoxes[GDat->ClusterNum[i]].NodeRect.TopLeft();
}

COLORREF	CharacterArchetype::GetStateColorFromNodeIndex(int idx)
{
	assert(idx >= 0);
	assert(idx < num_sequences);

	return	GDat->StateColors[sequences[idx].type];
}

void	CharacterArchetype::UpdateNodeColors(void)
{
	int	i;

	for(i=0;i < num_sequences;i++)
	{
		GDat->NodeBoxes[i].Color	=GDat->StateColors[sequences[i].type];
	}
}

GraphUIData	*CharacterArchetype::GetGraphData(void)
{
	return	GDat;
}


void	CharacterArchetype::CalcUIData(void)
{
	int				i, j, MaxXExtent;
	POINT			lpoints[64];
	MotionSequence	*ms;

	//get minimum box sizes for text
	//might be the wrong window here
	CDC	*pDC	=AfxGetMainWnd()->GetDC();
	for(j=0;j <= num_sequences / 64;j++)	//make sure don't overflow static
	{
		for(i=0;i < num_sequences;i++)
		{
			CSize	lsize	=pDC->GetTextExtent(sequences[i+(j * 64)].name);

			//this is annoying
			lpoints[i].x	=lsize.cx;
			lpoints[i].y	=lsize.cy;
		}
		
		LPtoDP(pDC->m_hDC, lpoints, num_sequences);

		//very annoying		
		for(i=0;i < num_sequences;i++)
		{
			int	idx	=i + (j * 64);

			GDat->NodeBoxes[idx].TextSize.cx		=lpoints[i].x;
			GDat->NodeBoxes[idx].TextSize.cy		=lpoints[i].y;
			GDat->NodeBoxes[idx].NodeRect.top		=0;
			GDat->NodeBoxes[idx].NodeRect.left		=0;
			GDat->NodeBoxes[idx].NodeRect.bottom	=lpoints[i].y;
			GDat->NodeBoxes[idx].NodeRect.right		=lpoints[i].x;
		}
	}

	//grab max text extents for state list
	for(i=0, MaxXExtent=-6969;i < MT_NUM_MOTION_TYPES;i++)
	{
		CSize	lsize	=pDC->GetTextExtent(MotionTypeNames[i]);

		if(MaxXExtent < lsize.cx)
		{
			MaxXExtent	=lsize.cx;
		}
		if(!GDat->StateColors[i])	//assign if unread
		{
			GDat->StateColors[i]	=RandomRGB();
		}
	}

	AfxGetMainWnd()->ReleaseDC(pDC);

	//index nodes into clusters
	memset(GDat->ClusterNum, -1, num_sequences * sizeof(int));
	GDat->CurCluster	=-1;

	for(i=0;i < num_sequences;i++)
	{
		if(GDat->ClusterNum[i] == -1)	//new cluster
		{
			GDat->ClusterNum[i]	=++GDat->CurCluster;
			FindAttached(i);
			if(sequences[i].end)
			{
				//reset visited
				for(j=0;j < num_sequences;j++)
				{
					sequences[j].bVisited	=FALSE;
				}
				for(ms=&sequences[i];ms;ms=ms->end)
				{
					if(ms->bVisited)
					{
						break;
					}
					ms->bVisited	=TRUE;
					int	idx	=(((int)ms - (int)sequences) / (int)sizeof(MotionSequence));
					if(GDat->ClusterNum[idx] == -1)
					{
						GDat->ClusterNum[idx]	=GDat->CurCluster;
						FindAttached(idx);
					}
				}
			}
		}
	}

	GDat->CurCluster++;

	if(GDat->ClusterBoxes)
	{
		delete	[]	GDat->ClusterBoxes;
	}

	GDat->ClusterBoxes	=new	NodeBox[GDat->CurCluster];

	//recurse into branches and find the depths from each node
	for(i=0;i < num_sequences;i++)
	{
		sequences[i].bVisited	=FALSE;
	}
	for(i=0;i < num_sequences;i++)
	{
		GDat->BranchDepths[i]	=GetBranchDepth(&sequences[i], 0);
	}

	for(i=0;i < GDat->CurCluster;i++)
	{
		CalcClusterExtents(i);
	}

	GDat->StateListBox.NodeRect.top		=0;
	GDat->StateListBox.NodeRect.left	=0;
	GDat->StateListBox.NodeRect.bottom	=GDat->NodeBoxes[0].TextSize.cy * MT_NUM_MOTION_TYPES;
	GDat->StateListBox.NodeRect.right	=MaxXExtent;
	GDat->StateListBox.TextSize.cy		=GDat->NodeBoxes[0].TextSize.cy * MT_NUM_MOTION_TYPES;
	GDat->StateListBox.TextSize.cx		=MaxXExtent;
}

//blows away everything just to add one sequence
//unsafe and should be redone for safety... dammit
void	CharacterArchetype::CreateNewSequence(void)
{
	int				i, ofs, j;
	int				*by		=new int[num_sequences+1];
	MotionSequence	*seq	=new MotionSequence[num_sequences+1];
	NodeBox			*nb		=new NodeBox[num_sequences+1];
	int				*cc		=new int[num_sequences+1];
	int				*cn		=new int[num_sequences+1];
	int				*bd		=new int[num_sequences+1];

	memcpy(seq, sequences, sizeof(MotionSequence) * num_sequences);
	memcpy(by, GDat->BranchY, sizeof(int) * num_sequences);
	memcpy(nb, GDat->NodeBoxes, sizeof(NodeBox) * num_sequences);
	memcpy(cc, GDat->ConnectionCount, sizeof(int) * num_sequences);
	memcpy(cn, GDat->ClusterNum, sizeof(int) * num_sequences);
	memcpy(bd, GDat->BranchDepths, sizeof(int) * num_sequences);

	ofs	=(int)sequences - (int)seq;

	for(i=0;i < num_sequences;i++)
	{
		if(seq[i].end)
		{
			*((int *)&seq[i].end)	-=ofs;
		}
	}
	for(i=0;i < MT_NUM_MOTION_TYPES;i++)
	{
		for(j=0;j < MT_NUM_MOTION_TYPES;j++)
		{
			if(seq_grid[i][j])
			{
				*((int *)seq_grid[i][j])	-=ofs;
			}
		}
	}

	seq[num_sequences].type					=MT_NONE;
	seq[num_sequences].name					=new char[80];
	seq[num_sequences].script				=new char[80];
	seq[num_sequences].end					=NULL;
	seq[num_sequences].loop					=FALSE;
	seq[num_sequences].reverse				=FALSE;
	seq[num_sequences].interrupt			=FALSE;
	seq[num_sequences].transition_duration	=0.0f;
	seq[num_sequences].num_wait_states		=0;
	seq[num_sequences].wait_events			=NULL;
	seq[num_sequences].wait_targets			=NULL;
	((char *)seq[num_sequences].script)[0]	=0;
	strcpy(((char *)seq[num_sequences].name), "NewSequence");

	free(sequences);
	free(GDat->BranchY);
	free(GDat->NodeBoxes);
	free(GDat->ConnectionCount);
	free(GDat->ClusterNum);
	free(GDat->BranchDepths);

	num_sequences++;

	sequences				=seq;
	GDat->BranchY			=by;
	GDat->NodeBoxes			=nb;
	GDat->ConnectionCount	=cc;
	GDat->ClusterNum		=cn;
	GDat->BranchDepths		=bd;
	GDat->num_sequences		=num_sequences;
	GDat->pSequences		=&seq;

	CalcUIData();
}

