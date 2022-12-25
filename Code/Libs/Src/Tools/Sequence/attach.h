#ifndef ATTACH_H
#define ATTACH_H

//

struct AttachmentArchetype
{
	const char *	name;			// attachment name, arbitrary.
	const char *	child_object;
	const char *	parent_hp;
	const char *	child_hp;
	bool			active:1;
	bool			select:1;

	AttachmentArchetype(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~AttachmentArchetype(void)
	{
		free((void *) name);
		name = NULL;
		free((void *) child_object);
		child_object = NULL;
		free((void *) parent_hp);
		parent_hp = NULL;
		free((void *) child_hp);
		child_hp = NULL;
	}
};

//

struct Attachment
{
	const AttachmentArchetype *	arch;
	bool						active;
	int							parent;
	int							child;
};

//

struct Transfer
{
	const char *name;
	const char *trigger;
	int			src;
	int			dst;

	Transfer(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~Transfer(void)
	{
		free((void *) name);
		name = NULL;
		free((void *) trigger);
		trigger = NULL;
	}

	void execute(class Character * c) const;
};

//

#endif