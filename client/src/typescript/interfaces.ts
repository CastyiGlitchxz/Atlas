export interface serverFormat {
    name: string;
    owner: string;
    serverID: string;
};

export interface Account {
    username: string;
    userid: string;
    email: string
};

export enum appearanceStatus {
    "online",
    "idle",
    "offline"
}

export interface Profile {
    displayName: string;
    status: appearanceStatus | ("online" | "offline" | "idle");
    picture?: string;
    customStatus?: string;
    userid: string;
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