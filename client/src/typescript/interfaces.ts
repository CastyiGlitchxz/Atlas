export const Client = {
    name: "Atlas Web Client",
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
export interface Profile {
    displayName: string;
    status: appearanceStatus | ("online" | "offline" | "idle");
    picture?: string;
    customStatus?: string;
    userID: string;
    bio: string;
};

export interface messageFormat {
    id: number;
    picture: string;
    displayName: string;
    serverID: string;
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

export interface serverInfo {
    icon: (null | string);
    owner: {
        user_id: string;
        username: string;
    }
    server_name: string;
}