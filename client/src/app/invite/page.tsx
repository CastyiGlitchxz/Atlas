import { Suspense } from "react"
import Invite from "./invite-client"

export default function InvitePage() {
  return (
    <Suspense fallback={<div>Loading invite…</div>}>
      <Invite />
    </Suspense>
  )
}
