'use client'

import React, { useEffect, useState } from "react"
import { construct_path } from "../../typescript/env";
import { get_token } from "../../typescript/user";
import { useRouter } from "next/navigation";
import { Client } from "../../typescript/interfaces";

export default function LogoutPage() {
    const router = useRouter();
    const [count, setCount] = useState<number>(5);
    const [timerState, setTimerState] = useState<boolean>(false);

    useEffect(() => {
        async function handle_logout() {
            const token = get_token();

            const res = await fetch(construct_path("api/logout"), {
                method: "GET",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`,
                    "Apikey": Client.apikey
                },
            });
            
            if (res.status === 200) {
                setTimerState(true);
                localStorage.removeItem("token");
                localStorage.removeItem("cached_userID");
            }
        }

        handle_logout();
    }, []);

    useEffect(() => {
        if (timerState === false) return;

        if (count <= 0) {
            router.replace("/");
            return;
        }

        const timer = setInterval(() => {
            setCount(prev => prev - 1);
        }, 1000);

        return () => clearInterval(timer);
    }, [count, timerState, router]);


    return (
        <div>
            <p>You&apos;ve been successfully logged out.</p>
            <p>Redirecting to login screen in {count} seconds.</p>
        </div>
    )
}