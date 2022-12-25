#ifndef CHARACTER_H
#define CHARACTER_H

//

#include <deform.h>
#include <ChannelTypes.h>
#include "object.h"
#include "motion.h"
#include "commands.h"
#include "attach.h"

//

struct CEdge
{
	int		v0, v1;		// indices into object vertex list.
	int		f0, f1;		// face indices.
	Vector	N;			// edge normal, average of face normals.

	CEdge(void)
	{
		v0 = v1 = -1;
		f0 = f1 = -1;
	}
};

//

typedef struct NodeBoxTag
{
	CRect		NodeRect;
	CSize		TextSize;
	COLORREF	Color;
} NodeBox;

//ui stuff for the graph stuff
typedef	struct	GraphUIDataTag
{
	int			num_sequences;
	NodeBox		*NodeBoxes, *ClusterBoxes, StateListBox;
	NodeBox		StateBoxes[MT_NUM_MOTION_TYPES];
	COLORREF	StateColors[MT_NUM_MOTION_TYPES];
	int			*ConnectionCount, *ClusterNum, *BranchDepths;
	int			CurCluster, *BranchY;
	MotionSequence	**pSequences;
}	GraphUIData;


class CharacterArchetype : public ObjectArchetype 
{
	public:
		void		CreateNewSequence(void);
		void		SetClusterColor(int cidx, COLORREF crgb);
		void		SetStateColor(int nidx, COLORREF crgb);
		void		CalcStateExtents(void);
		void		GetWorldSpaceNodeRect(CRect *ofsrect, int i);
		COLORREF	GetStateColorFromNodeIndex(int idx);
		void		UpdateNodeColors(void);
		void		CalcClusterExtents(int idx);
		void		SetNodesY(MotionSequence *ms, int y);
		void		FindAttached(int i);
		GraphUIData	*GetGraphData(void);
		void		CharacterArchetype::CalcUIData(void);

		CharacterArchetype(const char* name);
		~CharacterArchetype();
		BaseObject* create_instance();
		void delete_instance(BaseObject* obj);

		static BOOL32 __stdcall enum_callback(struct IProfileParser * parser, const C8 * sectionName, void *context);

		bool start_sequence(MotionType, class Character * c) const;

		inline float get_turn_rate(void) const
		{
			return turn_rate;
		}

	protected:
		ARCHETYPE_INDEX		arch_idx;


	public:
		float				turn_rate;
		char *				mesh_name;
		char *				anim_name;
		int					num_sequences;
		SCRIPT_SET_ARCH		anim_arch;
		int					num_scripts;
		const char **		script_names;

		GraphUIData			*GDat;
		MotionSequence *	sequences;
		MotionSequence *	seq_grid[MT_NUM_MOTION_TYPES][MT_NUM_MOTION_TYPES];

		int					num_edges;
		CEdge *				edges;

		int					num_attachments;
		AttachmentArchetype *attachments;

		int					num_transfers;
		Transfer *			transfers;
};

//

class Character : public GameObject, public Channel::IEventHandler
{
	public:

		DeformableObject *		deform;
		MotionType				motion;
		const MotionSequence *	current_sequence;
		const MotionSequence *	pending_sequence;
		int						change_state;
		float					yaw;

		Attachment *			attachments;

		bool					weapon_ready:1;
		bool					special:1;

		int						num_pending_transfers;
		const Transfer *		pending_transfers[8];

		class Gun *				selected_gun;
		HANDLE					aim, head_aim;

		Vector					p_IK;		// inverse kinematics targets.
		Matrix					R_IK;
		Matrix					Rhead;
		int						xIK, yIK;	// initial mouse position.

		INSTANCE_INDEX			head, neck;

		float					heading;
		float					last_height;
		Vector					last_pos;
		Vector					velocity;

		inline const CharacterArchetype * get_arch(void) const
		{
			return (const CharacterArchetype *) archetype;
		}

		Character(void);
		//Character(const char * ini_file, struct IFileSystem * data_fs);
		virtual ~Character(void);

		virtual Vector get_position(void);
		virtual void set_position(Vector& new_pos);

		virtual Matrix get_orientation();
		virtual void set_orientation(Matrix& new_orient);

		virtual void Render(struct ICamera * the_camera, U32 flags);
		virtual bool Update(float secs);

		virtual void command(CommandType command, U32 param = 0);

	// Channel::IEventHandler methods.
		virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator);

		virtual void COMAPI on_finished(unsigned int channel_id, void * user_supplied) {}
		virtual void COMAPI on_loop(unsigned int channel_id, Transform & xform, void * user_supplied) {}

		void attach(int idx, INSTANCE_INDEX child = INVALID_INSTANCE_INDEX);

		void start_aim(void);
		void end_aim(void);

	// DEBUG.
		void resupply(void);
};

//

#endif