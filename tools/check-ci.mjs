// -----------------------------------------------------------------------------
// check-ci.mjs - poll the latest GitHub Actions run for STGR IpTV and print
// each step's status until it completes.
//
//   node tools/check-ci.mjs [owner/repo] [--token <PAT>] [--once]
//
// Authentication: the script uses GITHUB_TOKEN or GH_TOKEN from the
// environment (or --token) if set. Authenticated requests have a much higher
// rate limit (5,000/hr vs 60/hr anonymous), so pass a token to avoid the
// anonymous API rate limit when polling repeatedly.
// -----------------------------------------------------------------------------

const DEFAULT_REPO = "Sensei002/stgr-iptv";
const INTERVAL_MS = 30000;
const MAX_POLLS = 8;

const args = process.argv.slice(2);
const repo = args.find((a) => !a.startsWith("--")) || DEFAULT_REPO;
const once = args.includes("--once");
const tokenIdx = args.indexOf("--token");
const token = tokenIdx >= 0 ? args[tokenIdx + 1] : process.env.GITHUB_TOKEN || process.env.GH_TOKEN || "";

const headers = { "User-Agent": "stgr-iptv-check-ci" };
if (token) headers["Authorization"] = `Bearer ${token}`;

async function api(path) {
    const res = await fetch(`https://api.github.com/repos/${repo}/${path}`, { headers });
    return res.json();
}

async function printSteps(runId) {
    const jobs = await api(`actions/runs/${runId}/jobs`);
    for (const j of jobs.jobs || []) {
        console.log(`  [${new Date().toISOString().slice(11, 19)}] job ${j.name}: ${j.status}${j.conclusion ? " -> " + j.conclusion : ""}`);
        for (const s of j.steps || []) {
            console.log(`      ${s.name}: ${s.conclusion || s.status}`);
        }
    }
}

async function main() {
    let lastRunId = null;
    for (let i = 0; i < MAX_POLLS; i++) {
        const runs = await api("actions/runs?per_page=3");
        if (runs.message) {
            console.error("API error:", runs.message);
            process.exitCode = 1;
            return;
        }
        const run = runs.workflow_runs?.[0];
        if (!run) {
            console.log("no runs found");
            return;
        }
        const head = String(run.head_commit?.message || "").slice(0, 55);
        if (i === 0 || run.id !== lastRunId) {
            console.log(`run ${run.id} | ${head} | ${run.status}${run.conclusion ? " -> " + run.conclusion : ""}`);
        }
        lastRunId = run.id;
        await printSteps(run.id);
        if (run.status === "completed" || once) {
            console.log(run.status === "completed" ? `RUN FINISHED: ${run.conclusion}` : "done (--once)");
            return;
        }
        await new Promise((r) => setTimeout(r, INTERVAL_MS));
    }
    console.log("still running after", MAX_POLLS, "polls");
}

main().catch((e) => {
    console.error("FATAL:", e.message);
    process.exitCode = 1;
});
