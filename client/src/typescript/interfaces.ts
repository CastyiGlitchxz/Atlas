export interface serverFormat {
    name: string;
    owner: string;
    serverID: string;
};

export interface Account {
    username: string;
    userid: string;
};

export interface Profile {
    displayName: string;
    status: "online" | "offline" | "idle";
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