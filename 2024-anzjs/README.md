# ANZJS paper: MATILDA metadata and instances

https://matilda.unimelb.edu.au/matilda/data-analytics

metadata.csv provides algo_fullstrong and algo_pscost: number of branch and bound
tree nodes for strong branching and pseudocosts, respectively. We are looking for
instances where pseudocosts (cheap proxy measure for the strong branching 'oracle')
performs particularly badly. So we use a much higher performance threshold than
would typically be applied in ISA.

- optimization criteria = minimize
- performance criteria = relative
- performance threshold = 5
- performance metric label = "Search Tree Nodes"


Footprint Analysis
Row	Area_Good_Normalized	Density_Good_Normalized	Purity_Good	Area_Best_Normalized	Density_Best_Normalized	Purity_Best
fullstrong	1	1	1	0.397	1.025	0.942
pscost	0.951	0.97	0.971	0.032	1.106	0.81
 
Average Performance Metric With SVM Accuracy and parameters
Algorithm	All Instances	Selected Instances
Avg. Performance
(Std. Performance)	Actual Percentage of "Good" Performances	Avg. Performance
(Std. Performance)	SVM Accuracy and Parameters
Overall Accuracy(%), Precision, Recall
(BoxConstraint, KernelScale)
fullstrong	305.485
( 1342.532 )	1	305.485
( 1342.532 )	200, 50, 50
( 0.001, 0.847 )
pscost	583.663
( 1794.227 )	0.893	154.191
( 911.308 )	87, 95.6, 89.6
( 0.218, 0.183 )
Oracle	299.473
( 1325.791 )	1		
Selector	367.124
( 1474.239 )	0.974	367.124
( 1474.239 )	—, 97.4, —
