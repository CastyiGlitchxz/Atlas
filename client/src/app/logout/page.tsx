'use client'

import { construct_path } from "../../typescript/env";
import { useEffect, useState } from "react"
import { get_token } from "../../typescript/user";
import { useRouter } from "next/navigation";
import { eventManager } from "../../typescript/eventsManager";

export default function LogoutPage() {
    const router = useRouter();
    const [count, setCount] = useState<number>(5);
    const [timerState, setTimerState] = useState<boolean>(false);

    useEffect(() => {
        async function handle_logout() {
            const em = new eventManager();
            const token = get_token();

            const res = await fetch(construct_path("api/logout"), {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": `Bearer ${token}`
                },
            });
            
            const data: {
                status: number,
                message: string
            } = await res.json();
            
            if (data.status === 200) {
                setTimerState(true);
                localStorage.removeItem("token");
            }

            em.emitEvent("update_status", { auth: token, status: "offline" });
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