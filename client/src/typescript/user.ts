import { construct_path } from "./env";
import { Profile } from "./interfaces";
import { Dispatch, SetStateAction } from "react";

export function get_token(): string {
    if (typeof document === "undefined")
        return null;

    const token = localStorage.getItem("token");
    if (!token) return "";

    return token;
};

export async function login_status(): Promise<boolean> {
    const token = get_token();

    const res = await fetch(construct_path("api/login_status"), {
        method: "GET",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`
        },
    });

    const data = await res.json();
    return data.logged_in;
};

export function open_profile(user: Profile, setPreview: Dispatch<SetStateAction<Profile>>, setShowPreview: Dispatch<SetStateAction<string>>) {
    setShowPreview("stack");
    setPreview(user);
};