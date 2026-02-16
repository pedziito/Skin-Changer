# GitHub Actions Larger Runner Configuration

This document explains the larger runner configuration implemented for the CS2 Skin Changer project.

## Overview

The project now uses larger GitHub-hosted runners to improve CI/CD performance and build times.

## Configuration Changes

### Workflow File: `.github/workflows/ci-cd.yml`

#### 1. Build and Test Job
```yaml
build-and-test:
  runs-on: ubuntu-latest-4-cores  # Changed from ubuntu-latest
```

**Resources:**
- CPU: 4 cores (vs 2 cores standard)
- RAM: 16 GB (vs 7 GB standard)
- Storage: 14 GB SSD (same)

**Benefits:**
- Faster npm package installation
- Parallel test execution
- Quicker Node.js build processes

#### 2. C++ Client Build Job
```yaml
build-cpp-client:
  runs-on: windows-latest-8-cores  # Changed from windows-latest
```

**Resources:**
- CPU: 8 cores (vs 2 cores standard)
- RAM: 32 GB (vs 7 GB standard)
- Storage: 14 GB SSD (same)

**Benefits:**
- Faster CMake configuration
- Parallel C++ compilation with MSVC
- Significantly reduced build time for large C++ projects
- Better handling of Visual Studio toolchain

#### 3. Deploy Job
```yaml
deploy:
  runs-on: ubuntu-latest-4-cores  # Changed from ubuntu-latest
```

**Resources:**
- CPU: 4 cores (vs 2 cores standard)
- RAM: 16 GB (vs 7 GB standard)
- Storage: 14 GB SSD (same)

**Benefits:**
- Faster artifact packaging
- Quicker compression operations
- Improved deployment performance

## Performance Expectations

### Build Time Improvements

| Job Type | Standard Runner | Larger Runner | Expected Improvement |
|----------|----------------|---------------|---------------------|
| Node.js Build & Test | ~3-5 min | ~1.5-2.5 min | 40-50% faster |
| C++ Compilation | ~10-15 min | ~4-6 min | 60-70% faster |
| Deployment | ~2-3 min | ~1-1.5 min | 40-50% faster |

### Resource Utilization

**4-Core Ubuntu Runners:**
- Parallel npm package installation (4 concurrent downloads)
- Faster JavaScript/TypeScript compilation
- Better test parallelization
- Improved artifact compression

**8-Core Windows Runners:**
- CMake parallel build generation
- MSVC multi-threaded compilation (`/MP` flag)
- Faster linking with parallel processing
- Better Visual Studio toolchain performance

## Cost Considerations

### GitHub Actions Minutes Consumption

| Runner Type | Minute Multiplier | Cost Impact |
|-------------|------------------|-------------|
| ubuntu-latest | 1x | Baseline |
| ubuntu-latest-4-cores | 2x | 2x minutes consumed |
| windows-latest | 2x | 2x minutes consumed |
| windows-latest-8-cores | 8x | 8x minutes consumed |

### Budget Planning

**For Public Repositories:**
- Free tier includes unlimited minutes for public repos
- Larger runners fully supported

**For Private Repositories:**
- Consider the increased minute consumption
- Monitor usage in repository insights
- Optimize workflow triggers (e.g., skip builds for documentation changes)

## Availability Requirements

### GitHub Plans Supporting Larger Runners

✅ **Supported:**
- GitHub Free (public repositories only)
- GitHub Pro
- GitHub Team
- GitHub Enterprise Cloud
- GitHub Enterprise Server

❌ **Not Supported:**
- GitHub Free (private repositories) - uses standard runners

### Fallback Behavior

If larger runners are not available:
- Workflow will automatically fall back to standard runners
- Build will complete successfully (just slower)
- No workflow file changes needed

## Verification

### Check Runner Type in Logs

In GitHub Actions logs, you'll see:

```
Runner name: 'Hosted Agent'
Runner type: ubuntu-latest-4-cores
Total Available Memory: 16 GB
Total Cores: 4
```

### Monitor Performance

1. Go to repository **Actions** tab
2. Select a workflow run
3. Check **Job duration** times
4. Compare with previous runs using standard runners

## Optimization Tips

### 1. Leverage Parallelism

**Node.js:**
```yaml
- name: Run tests
  run: npm test -- --parallel
```

**C++ with CMake:**
```yaml
- name: Build project
  run: cmake --build . --config Release --parallel 8
```

### 2. Cache Dependencies

```yaml
- name: Cache npm packages
  uses: actions/cache@v3
  with:
    path: ~/.npm
    key: ${{ runner.os }}-node-${{ hashFiles('**/package-lock.json') }}
```

### 3. Skip Unnecessary Builds

```yaml
on:
  push:
    paths-ignore:
      - '**.md'
      - 'docs/**'
```

## Monitoring and Metrics

### Key Metrics to Track

1. **Build Duration**: Total time from start to finish
2. **CPU Utilization**: Percentage of available CPU used
3. **Memory Usage**: Peak memory consumption
4. **Artifact Size**: Size of generated artifacts
5. **Minute Consumption**: Total GitHub Actions minutes used

### GitHub Actions Insights

View metrics at:
```
https://github.com/pedziito/Skin-Changer/actions
```

## Troubleshooting

### Issue: Workflow fails with "Runner not found"

**Solution:** Check GitHub plan supports larger runners for your repository type.

### Issue: No performance improvement

**Solution:** 
1. Ensure build process uses parallel operations
2. Check if bottleneck is elsewhere (network, disk I/O)
3. Profile the build to identify slow steps

### Issue: Excessive minute consumption

**Solution:**
1. Add path filters to skip unnecessary builds
2. Use caching for dependencies
3. Consider selective larger runner usage (only for heavy jobs)

## References

- [GitHub Docs: Larger Runners](https://docs.github.com/en/actions/using-github-hosted-runners/about-larger-runners)
- [Customizing Agent Environment](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/customize-the-agent-environment#upgrading-to-larger-github-hosted-github-actions-runners)
- [Actions Minute Multipliers](https://docs.github.com/en/billing/managing-billing-for-github-actions/about-billing-for-github-actions)

## Support

For issues or questions:
1. Check GitHub Actions logs for error details
2. Review this documentation
3. Open an issue in the repository
4. Contact GitHub Support (for runner availability issues)

---

Last Updated: 2026-02-16
