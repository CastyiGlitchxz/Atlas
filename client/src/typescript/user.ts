import { construct_path } from "./env";
import { Client, Profile } from "./interfaces";
import { Dispatch, SetStateAction } from "react";

export function get_token(): string {
    if (typeof document === "undefined")
        return null;

    const token = localStorage.getItem("token");
    if (!token) return "";

    return token;
};

export function getUserIDCache(): (string | undefined) {
    const id: (string | undefined) = window.localStorage.getItem("cached_userID");

    if (!id || id === undefined || id === null) {
        return undefined;
    }

    return id;
}

export function setUserIDCache(userID: string): void {
    const exist: string = getUserIDCache();

    if (exist === undefined) {
        window.localStorage.setItem("cached_userID", userID);
    }
}

export async function login_status(): Promise<boolean> {
    const token = get_token();

    const res = await fetch(construct_path("api/login_status"), {
        method: "GET",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`,
            "Apikey": Client.apikey,
        },
    });

    const data = await res.json();
    return data.logged_in;
};

export function open_profile(user: Profile, setPreview: Dispatch<SetStateAction<Profile>>, setShowPreview: Dispatch<SetStateAction<string>>) {
    setShowPreview("stack");
    setPreview(user);
};