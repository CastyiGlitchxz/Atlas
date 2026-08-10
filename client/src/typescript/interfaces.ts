export const Client = {
    apikey: process.env.NEXT_PUBLIC_APIKEY,
}

export interface serverFormat {
    name: string;
    owner: string;
    serverID: string;
    icon: string | null;
};

export interface Account {
    username: string;
    userID: string;
    email: string;
};

export enum appearanceStatus {
    "online",
    "idle",
    "offline"
}

export enum relationshipType {
    accepted,
    pending,
    blocked
}

export interface Friends {
    friends: Friend[];
    requests: Friend[];
}

export interface Friend extends Profile {
    id: number;
}

export interface clientINT {
    clientName: string;
    internalSlug: string;
    isTrusted: boolean;
}

export interface Profile {
    displayName: string;
    status: appearanceStatus | ("online" | "offline" | "idle");
    picture?: string;
    customStatus?: string;
    userID: string;
    bio: string;
    client?: clientINT;
}

export interface IMessageFormat {
    id: number;
    picture: string;
    displayName: string;
    channelID: string;
    groupID: string;
    receiverID: string;
    content: string;
    timestamp?: string;
    messageRef: number;
    link?: string;
    userID: string;
};

export interface loginResponse {
    response: {
        status: number;
        message: string;
        token: string;
    };
    error?: string;
};

export interface IChannel {
    channelID: bigint;
    channelName: string;
}

export interface IServerData {
    icon: (null | string);
    owner: {
        userID: string;
        displayName: string;
        avatar: string;
    }
    serverName: string;
    channels: IChannel[]
}
