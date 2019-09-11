
import contextlib


def parse_logs_raw(lines):
    ''' Basic section/subsection breakdown of SCIP statistics log. '''

    statistics = {}
    section = None
    subsection = None

    for line in lines:

        if line.startswith('SCIP Status'):
        # if line.strip() == 'Statistics':
            section = 'stats'

        if section == 'stats':

            label, mid, values = line.partition(':')
            if mid != ':':
                continue
            label = label.rstrip()
            values = values.strip()

            if label.startswith(' '):
                statistics[subsection][label.lstrip()] = values
            else:
                subsection = label
                statistics[subsection] = {'data': values}

    return statistics


def convert_simple(value):
    if value.strip() == '-':
        return None
    try:
        return int(value)
    except ValueError:
        try:
            return float(value)
        except ValueError:
            return value


def convert_split(value):
    value, bracket, extra = value.partition('(')
    extra = extra.replace(')', '').strip()
    if bracket and not (extra.endswith('%') or extra.endswith('Iter/sec')):
        return convert_simple(value), extra
    return convert_simple(value)


def parse_grid_section(section):
    header = section['data'].split()
    return {
        row: dict(zip(header, map(convert_simple, row_values.split())))
        for row, row_values in section.items()
        if row != 'data'
    }


def parse_simple_section(section):
    return {
        normalise_name(key): convert_split(value)
        for key, value in section.items()
        if key != 'data'
    }


def normalise_name(name):
    name = name.lower().replace(' ', '_')
    name = name.replace('(', '').replace(')', '')
    name = name.replace('infeas.', 'infeasible')
    name = name.replace('avg.', 'average')
    if name == 'b&b_tree':
        return 'tree'
    return name


def parse_logs(log_data):

    parsed_raw  = parse_logs_raw(log_data.split('\n'))

    statistics = {
        'solve_status': parsed_raw['SCIP Status']['data'],
    }

    section = parsed_raw['Total Time']
    statistics['timing'] = {
        'total' if key == 'data' else key: float(value.split()[0])
        for key, value in section.items()
    }

    section = parsed_raw['Original Problem']

    for section_name in [
        'Presolvers', 'Constraints', 'Constraint Timings', 'Propagators', 'Propagator Timings',
        'Conflict Analysis', 'Separators', 'Pricers', 'Branching Rules', 'Primal Heuristics',
        'Diving Statistics', 'Neighborhoods', 'LP'
        ]:
        statistics[normalise_name(section_name)] = parse_grid_section(parsed_raw[section_name])

    for section_name in [
        'B&B Tree', 'Root Node', 'Solution'
        ]:
        statistics[normalise_name(section_name)] = parse_simple_section(parsed_raw[section_name])

    tree_stats = statistics['tree']
    nodes, extra = tree_stats['nodes']
    internal_nodes, _, leaf_nodes, _ = extra.split()
    tree_stats['nodes'] = nodes
    tree_stats['internal_nodes'] = convert_simple(internal_nodes)
    tree_stats['leaf_nodes'] = convert_simple(leaf_nodes)
    nodes, extra = tree_stats['nodes_total']
    internal_nodes, _, leaf_nodes, _ = extra.split()
    tree_stats['nodes_total'] = nodes
    tree_stats['internal_nodes_total'] = convert_simple(internal_nodes)
    tree_stats['leaf_nodes_total'] = convert_simple(leaf_nodes)
    tree_stats['repropagations'] = tree_stats['repropagations'][0]

    solution_stats = statistics['solution']
    solutions_found, improvements = solution_stats['solutions_found']
    solution_stats['solutions_found'] = solutions_found
    solution_stats['improvements'] = convert_simple(improvements.split()[0])
    with contextlib.suppress(KeyError):
        solution_stats['first_solution'] = solution_stats['first_solution'][0]
    with contextlib.suppress(KeyError):
        solution_stats['primal_bound'] = solution_stats['primal_bound'][0]
    average_gap, primal_dual_integral = solution_stats['average_gap']
    solution_stats['average_gap'] = average_gap
    solution_stats['primal_dual_integral'] = convert_simple(primal_dual_integral.split()[0])

    del statistics['separators']['cut pool']

    # print any remaining tuples (unprocessed split values)
    # for section, section_data in statistics.items():
    #     if type(section_data) is dict:
    #         for subsection, value in section_data.items():
    #             if type(value) is tuple:
    #                 print(section, subsection, value)

    return statistics
