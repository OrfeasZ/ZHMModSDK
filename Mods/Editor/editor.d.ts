// Common types.
type HexDigit = '0' | '1' | '2' | '3' | '4' | '5' | '6' |
		'7' | '8' | '9' | 'A' | 'B' | 'C' | 'D' |
		'E' | 'F' | 'a' | 'b' | 'c' | 'd' | 'e' |
		'f';

type NullByte = '00';
type HexByte = string; // `${HexDigit}${HexDigit}`

// The TS compiler cannot handle the union cases below when HexByte is defined
// correctly, so we are forced to use a string instead until better pattern
// matching support is added.

type HexInt16 = `${HexByte}${HexByte}`;
type HexInt32 = `${HexInt16}${HexInt16}`;
type HexInt56 = `${HexByte}${HexInt16}${HexInt32}`;
type HexInt64 = `${HexInt32}${HexInt32}`;

// 64-bit integer encoded as a hex string.
type EntityId = HexInt64;

// 56-bit integer represented in 64-bits, and encoded as a hex string.
// This means that this always starts with 00.
// e.g. 00AEAFEE252AE850
type ResourceId = `${NullByte}${HexInt56}`;

interface Vec3 {
	x: number;
	y: number;
	z: number;
}

// Rotation in radians.
interface Rotation {
	yaw: number;
	pitch: number;
	roll: number;
}

interface Transform {
	position: Vec3;
	rotation: Rotation;
	scale: Vec3;
}

interface PropertyValue {
	// The type of the property value.
	type: string;

	// The data of the property, as it would appear in RT JSON.
	data: unknown;
}

interface GameEntity {
	id: EntityId;
	tblu: ResourceId;
}

interface EditorEntity {
	id: EntityId;
	byEditor: true;
}

// Rules used to select either an entity spawned by the game or by the editor.
// When `tblu` is defined and set to a blueprint (TBLU) resource id / hash,
// the entity will be looked up based on the provided `id` in the collection of entities
// spawned by the game based on that blueprint.
// Otherwise, when `byEditor` is defined and set to `true`, the entity will be looked up
// in the collection of entities spawned by the editor at runtime.
type EntitySelector = GameEntity | EditorEntity;


interface EntityBaseData {
	name?: string;
	type: string;
}

type GameEntityBaseDetails = GameEntity & EntityBaseData;
type EditorEntityBaseDetails = EditorEntity & EntityBaseData;
type EntityBaseDetails = GameEntityBaseDetails | EditorEntityBaseDetails;

// For properties with unknown names, their name will be formatted as such:
// ~id
// where `id` is a 16-character hexadecimal representation of the property id, which
// is the CRC32 hash of the property name.
type UnknownPropertyName = `~${HexInt32}`;
type PropertyName = string | UnknownPropertyName;

interface EntityData extends EntityBaseData {
	parent?: EntitySelector;
	transform?: Transform;
	relativeTransform?: Transform;
	properties: Record<PropertyName, PropertyValue>;
	interfaces: string[];
}

type GameEntityDetails = GameEntity & EntityData;
type EditorEntityDetails = EditorEntity & EntityData;
type EntityDetails = GameEntityDetails | EditorEntityDetails;


// Requests from a third party program to the editor.
declare namespace EditorRequests {
	// A hello message, which must be sent when the connection to the editor is
	// first established.
	interface Hello {
		type: 'hello';

		// The name of the third party program.
		identifier: string;
	}

	interface SelectEntity {
		type: 'selectEntity';

		// The entity to select.
		entity: EntitySelector;
	}

	interface SetEntityTransform {
		type: 'setEntityTransform';

		// The entity to set the transform of.
		entity: EntitySelector

		// The new transform of the entity.
		transform: Transform;

		// `true` to set the transform relative to the parent, `false` to set it in world space.
		relative: boolean;
	}

	interface SpawnEntity {
		type: 'spawnEntity';

		// The id of the entity template (TEMP hash).
		templateId: ResourceId;

		// The id to assign to the new entity.
		entityId: EntityId;

		// A human-readable name for the entity.
		name: string;
	}

	interface DestroyEntity {
		type: 'destroyEntity';

		// The entity to destroy.
		entity: EntitySelector;
	}

	interface SetEntityName {
		type: 'setEntityName';

		// The entity to set the name of.
		entity: EntitySelector;

		// The new name of the entity.
		name: string;
	}

	interface SetEntityProperty {
		type: 'setEntityProperty';

		// The entity to set the property of.
		entity: EntitySelector;

		// The name or id of the property to set.
		property: string | number;

		// The new value to give to the property, as it would appear in RT JSON.
		value: unknown;
	}

	interface SignalEntityPin {
		type: 'signalEntityPin';

		// The entity to signal the pin of.
		entity: EntitySelector;

		// The name or id of the pin to signal.
		pin: string | number;

		// `true` if an output pin should be signaled, `false` if an input pin should be signaled.
		output: boolean;
	}

	// Request a list of spawned entities.
	interface ListEntities {
		type: 'listEntities';

		// `true` to only list entities spawned by the editor, `false` or undefined to list all entities.
		editorOnly?: boolean;
	}

	interface GetEntityDetails {
		type: 'getEntityDetails';

		// The entity to get the details of.
		entity: EntitySelector;
	}

	interface GetHitmanEntity {
		type: 'getHitmanEntity';
	}

	interface GetCameraEntity {
		type: 'getCameraEntity';
	}

	interface RebuildEntityTree {
		type: 'rebuildEntityTree';
	}
}

type EditorRequest =
		EditorRequests.Hello
		| EditorRequests.SelectEntity
		| EditorRequests.SetEntityTransform
		| EditorRequests.SpawnEntity
		| EditorRequests.DestroyEntity
		| EditorRequests.SetEntityName
		| EditorRequests.SetEntityProperty
		| EditorRequests.SignalEntityPin
		| EditorRequests.ListEntities
		| EditorRequests.GetEntityDetails
		| EditorRequests.GetHitmanEntity
		| EditorRequests.GetCameraEntity
		| EditorRequests.RebuildEntityTree;

// Events from the editor to a third party program.
declare namespace EditorEvents {
	// A welcome message, which is sent in response to a hello message.
	interface Welcome {
		type: 'welcome';
	}

	interface Error {
		type: 'error';

		// The error message.
		message: string;
	}

	interface EntitySelected {
		type: 'entitySelected';

		// The entity that was selected.
		entity: EntityDetails;
	}

	interface EntityDeselected {
		type: 'entityDeselected';
	}

	interface EntityTransformUpdated {
		type: 'entityTransformUpdated';

		// The entity whose transform was updated.
		entity: EntityDetails;
	}

	interface EntityNameUpdated {
		type: 'entityNameUpdated';

		// The entity whose name was updated.
		entity: EntityDetails;
	}

	interface EntitySpawned {
		type: 'entitySpawned';

		// The entity that was spawned.
		entity: EntityDetails;
	}

	interface EntityDestroyed {
		type: 'entityDestroyed';

		// The id of the destroyed entity.
		entity: EntitySelector;
	}

	interface EntityPropertyChanged {
		type: 'entityPropertyChanged';

		// The entity that was updated.
		entity: EntityDetails;

		// The name or id of the property that was updated.
		property: string | number;

		// The new value of the property.
		value: PropertyValue;
	}

	interface SceneLoading {
		type: 'sceneLoading';

		// The name of the scene that is being loaded.
		scene: string;

		// The name of additional bricks that are being loaded.
		bricks: string[];
	}

	// The scene is being cleared (e.g. for a reload or a switch to a different scene).
	interface SceneClearing {
		type: 'sceneClearing';

		// Whether the scene is being cleared for a level reload.
		// If `false`, it means the game will transition to a different scene.
		forReload: boolean;
	}

	interface EntityListResponse {
		type: 'entityList';

		// The list of requested entities.
		entities: EntityBaseDetails[];
	}

	interface EntityDetailsResponse {
		type: 'entityDetails';

		// The details of the requested entity.
		entity: EntityDetails;
	}

	interface HitmanEntityResponse {
		type: 'hitmanEntity';

		// The details of the Hitman entity.
		entity: EntityDetails;
	}

	interface CameraEntityResponse {
		type: 'cameraEntity';

		// The details of the active camera entity.
		entity: EntityDetails;
	}

	interface EntityTreeRebuilt {
		type: 'entityTreeRebuilt';
	}
}

type EditorEvent =
		EditorEvents.Welcome
		| EditorEvents.Error
		| EditorEvents.EntitySelected
		| EditorEvents.EntityDeselected
		| EditorEvents.EntityTransformUpdated
		| EditorEvents.EntityNameUpdated
		| EditorEvents.EntitySpawned
		| EditorEvents.EntityDestroyed
		| EditorEvents.EntityPropertyChanged
		| EditorEvents.SceneLoading
		| EditorEvents.SceneClearing
		| EditorEvents.EntityListResponse
		| EditorEvents.EntityDetailsResponse
		| EditorEvents.HitmanEntityResponse
		| EditorEvents.CameraEntityResponse
		| EditorEvents.EntityTreeRebuilt;